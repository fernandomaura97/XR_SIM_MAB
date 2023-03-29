/*
	Poisson source. 
*/

#ifndef _XRServer_
#define _XRServer_

#include <vector>
#include <algorithm>	
#include "definitions.h"

#define ADAPTIVE_HEUR 0 		//set to 1 for heuristic adaptive control

#define MAXLOAD 10E7 			// max load for reward calcs



component XRServer : public TypeII
{
	
	public: // Default functions
		void Setup();
		void Start();
		void Stop();

		//utilities
		int overuse_detector(double mowdg, double threshold); //function to detect if mowdg is within limits of threshold. 

	public: // Connections
		outport void out(data_packet &packet);
		inport void in(data_packet &packet);	

		// Timer
		Timer <trigger_t> inter_video_frame;
		Timer <trigger_t> inter_packet_timer;
		Timer <trigger_t> rate_control;

		inport inline void new_video_frame(trigger_t& t); // action that takes place when timer expires
		inport inline void new_packet(trigger_t& t); // action that takes place when timer expires
		inport inline void AdaptiveVideoControl(trigger_t& t); // action that takes place when timer expires
		inport inline void GreedyControl(trigger_t& t);
		inport inline void QLearning(trigger_t& t);

		XRServer () { 
			connect inter_video_frame.to_component,new_video_frame; 
			connect inter_packet_timer.to_component,new_packet;
			connect rate_control.to_component,AdaptiveVideoControl;}


	public: // Input parameters
		int id; // sender
		int L_data;
		int destination;
		double Load; // bps
		int state; // current state
		int current_action = 0;
		double fps;
		int node_attached;
		int source_app;
		int destination_app;
		double last_frame_generation_time;
		int rate_control_activated;

		int rtt_counter; 
		double QoE_metric; 


	private:
		double tau; //
		double inter_frame_time;
		int NumberPacketsPerFrame, auxNumberPacketsPerFrame;
		int tx_packets_per_frame;
		double video_frame_sequence=-1;


	public:
		double generated_packets=0;
		double received_packets=0;
		double generated_video_frames = 0;
		double avRTT = 0;
		double avRxFrames = 0;
		double p_control = 0;
		double new_load = 0;
		double controlRTT = 0;
		double rx_packet_controlRTT=0;
		
		double av_Load=0;
		double load_changes = 0;
		
		double MAB_rewards[10];
		int current_action = 0;
		double sent_frames_MAB = 0;
		double received_frames_MAB = 0;
		double RTT_MAB = 0;
		double jitter_sum_quadratic = 0; 
		double jitter_metric = 0; 

		double packet_loss_ratio = 0; 
		double rw_pl;
		double rw_threshold; 

		int signal_overuse; 

		
		double m_owdg; // measure of filtered delay gradient 

		struct csvfer_t{    ///TODO: CSV OUTPUTS
			double mean_frame_delay_;
			double frame_perc_99_;

			double mean_packet_delay_;
			double packet_delay_99_;
			double ratio_frames_;

			double MAB_r[10];
			int current_action = 0;
			double sent_frames_MAB = 0;
			double received_frames_MAB = 0;
			double RTT_MAB = 0;
		}csv_fer;

		std::vector <csvfer_t> vector_csv;
		std::vector<std::vector<double>> Q(STATE_SIZE, std::vector<double>(ACTION_SIZE));

};

void XRServer :: Setup()
{
	printf("XR Server Setup()\n");
};

void XRServer :: Start()
{
	//fps = 3 * fps; // to implement the idea of dividing a video frame in mini batches. It does not seem to work.

	NumberPacketsPerFrame = ceil((Load/L_data)/fps);

	tau = (double) L_data/Load;
	inter_frame_time = (double) 1 / fps;
	printf("%f\n",tau);
	inter_video_frame.Set(SimTime()+Exponential(150E-3));	
	if(rate_control_activated) rate_control.Set(SimTime()+0.5+Exponential(0.1));
		
	new_load = Load;

	printf("XR Server Start() : Load %f | FPS = %f | Packets = %d\n",Load,fps,NumberPacketsPerFrame);

	for (int r=0;r<10;r++)
	{
		MAB_rewards[r]=0.0;
		printf("%f ",MAB_rewards[r]);
	}
};
	
void XRServer :: Stop()
{
	printf("------------------ XR Server %d ------------------\n",id);
	printf("Generated Packets = %f | Received Packets = %f\n",generated_packets,received_packets);
	printf("Average RTT (last packet video frame) = %f \n",avRTT/rx_packet_controlRTT);
	printf("Average Load = %f\n",av_Load/generated_video_frames);
	printf("Number of Changes = %f | Rate of changes = %f\n",load_changes,load_changes/SimTime());


	//MAKE EXCEL HERE



};

void XRServer :: new_video_frame(trigger_t &)
{
	if(traces_on) printf("%f - XR Server %d : New video frame --------------------------------------\n",SimTime(),id);
	video_frame_sequence++;
	last_frame_generation_time = SimTime();
	tx_packets_per_frame = NumberPacketsPerFrame;
	auxNumberPacketsPerFrame = NumberPacketsPerFrame;
	inter_packet_timer.Set(SimTime()+10E-6);
	generated_video_frames++;	
	sent_frames_MAB++;

	av_Load += Load;

	/*
	if(test_average_delay_decision[id] > (double) 1/fps)
	{
		fps=MAX(30,fps/2);
	}
	else
	{
		fps=MIN(240,2*fps);

	}

	*/
	//printf("%f - XR server %d : Average Delay = %f | fps = %f | Time = %f | Load (new) = %f\n",SimTime(),id,test_average_delay_decision[id],fps,(double)1/fps,new_load);


	inter_video_frame.Set(SimTime()+inter_frame_time); // New frame

	/*
	if(Random()>p_control)
	{	
		inter_video_frame.Set(SimTime()+inter_frame_time);
	}
	else
	{
		inter_video_frame.Set(SimTime()+2*inter_frame_time);
	}
	*/	
	// TO study if randomization helps	
	//inter_video_frame.Set(SimTime()+(double)inter_frame_time/2+2*Random((double)inter_frame_time/2));
	
}

void XRServer :: new_packet(trigger_t &)
{
	//printf("%f - XR server %d : New packet\n",SimTime(),id);
	generated_packets++;
	data_packet XR_packet;
	XR_packet.L_data = L_data;
	XR_packet.L_header = 20*8; //IP, MAC headers in the sim
	XR_packet.L = 20*8 + L_data;

	XR_packet.source = node_attached;
	XR_packet.destination = destination;
	XR_packet.source_app = source_app;
	XR_packet.destination_app = destination_app;
	XR_packet.sent_time = SimTime();
	XR_packet.frame_generation_time = last_frame_generation_time;

	rtt_counter +=1;
	
	/*            ///ROUTINE TO MAKE 1 PACKET IN N BE FOR FEEDBACK AND KALMAN (UNUSED)
	if (rtt_counter >=10){
		rtt_counter = 0;
		XR_packet.rtt = true;
		XR_packet.send_time = SimTime() 
	}
	else{XR_packet.rtt = false;}
	*/

	if(tx_packets_per_frame == auxNumberPacketsPerFrame) 
	{
		XR_packet.first_video_frame_packet = 1;
	}
	else 
	{
		XR_packet.first_video_frame_packet = 0;
	}
	if(tx_packets_per_frame==1) 
	{
		XR_packet.last_video_frame_packet = 1;
	}
	else 
	{
		XR_packet.last_video_frame_packet = 0;
	}
	XR_packet.num_packet_in_the_frame = auxNumberPacketsPerFrame - tx_packets_per_frame + 1; 
	XR_packet.NumPacketsPerFrame = auxNumberPacketsPerFrame;
	XR_packet.video_frame_seq = video_frame_sequence;

	out(XR_packet);

	tx_packets_per_frame--;

	// Constant time between packets	
	if(tx_packets_per_frame > 0) inter_packet_timer.Set(SimTime()+10E-6);

};

void XRServer :: in(data_packet &packet)
{
	if(traces_on) printf("%f - XR server %d : Uplink Packet received\n",SimTime(),id);
	// Compute RTT & losses
	if(packet.feedback ==true){
		
		//jitter_sum_quadratic += ((packet.m_owdg)**2); //Quadratic sum of value
		int signal_overuse = overuse_detector( packet.m_owdg, packet.threshold_gamma);

		if(signal_overuse == 1){ 
			rw_threshold = 1;	//NORMAL IS REWARDED 4 TIMES AS UNDERUSE OR OVERUSE, FOR STABILITY
		}
		else if (signal_overuse == 2){
			printf("overuse");
			rw_threshold = 0.25;
		}
		else if(signal_overuse == 0){
			rw_threshold = 0.25; 
		}

		printf("Quadratic sum_jitter: %f, mowdg: %f", jitter_sum_quadratic, packet.m_owdg); 
	}

	if(packet.last_video_frame_packet == 1)
	{
		double RTT = SimTime() - packet.TimeSentAtTheServer;
		avRTT += RTT;
		//if(traces_on) 
		if(traces_on) printf("%f - XR server %d : Uplink Packet received: RTT = %f\n",SimTime(),id,RTT);

		controlRTT = (controlRTT + RTT)/2;
		rx_packet_controlRTT++;

		RTT_MAB = (RTT_MAB + RTT)/2;   					
												
		avRxFrames = (avRxFrames + packet.frames_received)/2;

		received_frames_MAB++;

		m_owdg = packet.m_owdg; //kalman filter estimate of One Way Delay Gradient!

		
		double packet_loss_ratio = 1 - received_frames_MAB/sent_frames_MAB;
		
		if(packet_loss_ratio<0.95){ 
			rw_pl = 0;
			}
		else if (packet_loss_ratio > 0.95) {
			rw_pl = Load/MAXLOAD; 				
		}

		//double QoS = ;  

		//QoE_metric = 3.01 * exp( -4.473 * )

		jitter_sum_quadratic = 0; 

	}
	/* IF WE UPDATE KALMAN FILTER FROM DIFFERENT ROUTINE, USE THIS INSTEAD 
	if(packet.feedback == true){
		m_owdg = packet.m_owdg; 

	}
	*/

	received_packets++;

};

void XRServer :: GreedyControl(trigger_t& t)
{
	
	// 2) Next Action
	int next_action = -1;
	if(Random()<=0.25)
	{
		// Explore
		printf("***************** EXPLORE ****************************\n");
		next_action = Random(2);  
		
	}
	else
	{
		printf("***************** EXPLOIT ****************************\n");

		// Get the maximum 
		int index_max = 0;
		double max_reward = MAB_rewards[0];
		for (int r=0;r<10;r++)
		{
			printf("%d %f\n",r,MAB_rewards[r]);
			if(max_reward < MAB_rewards[r])
			{
				index_max = r;
				max_reward = MAB_rewards[r];
			}
		}
		printf("The action with max reward is %d\n",index_max);
		next_action = index_max;
		
	}

	Load = 10E6*(next_action+1);
};

// Define the update function
void update(std::vector<std::vector<double>>& Q, int state, int action, double reward, int next_state) {
    double old_value = Q[state][action];
    double next_max = *max_element(Q[next_state].begin(), Q[next_state].end());
    double new_value = (1 - ALPHA) * old_value + ALPHA * (reward + GAMMA * next_max);
    Q[state][action] = new_value;
}
void reward(int a, int b){}
void XRServer :: QLearning(trigger_t& t)
{
        // Reset the current state to 0
        state = 0;

        // Loop over steps in the episode
        while (state != STATE_SIZE - 1) {
            // Choose an action using an epsilon-greedy policy
            int next_action = 0;
            double epsilon = 0.1;

            if(Random()<=0.25)
			{
				// Explore
				// printf("***************** EXPLORE ****************************\n");
				next_action = Random(2);
		
			} 	
			else
			{
				printf("***************** EXPLOIT ****************************\n");

                // Choose the action with the highest Q-value
				for(int a = 0, a < ACTION_SIZE, a++)
				{
                next_action = Q[state][a] > Q[state][current_action] ? a : current_action;
				}
            }

            // Calculate the reward and next state
            double r = reward(state, next_action);
            int next_state = action == 0 ? state + 1 : state - 1;

            // Update the Q-value for the current state-action pair
            update(Q, state, next_action, r, next_state);

            // Update the current state
			current_action = next_action;
            state = next_state;

		// {
		// 			// Get the maximum 
		// int index_max = 0;
		// double max_reward = MAB_rewards[0];
		// for (int r=0;r<10;r++)
		// {
		// 	printf("%d %f\n",r,MAB_rewards[r]);
		// 	if(max_reward < MAB_rewards[r])
		// 	{
		// 		index_max = r;
		// 		max_reward = MAB_rewards[r];
		// 	}
		// }
		// printf("The action with max reward is %d\n",index_max);
		// next_action = index_max;
		// }
        // }
    
	Load = 10E6*(state+1);
};

void XRServer :: AdaptiveVideoControl(trigger_t & t)
{
	#if ADAPTIVE_HEUR==1
  /*
       if(traces_on) 
       printf("%f - XR server %d : Rate Control ------------------- with Losses = %f | RTT = %f\n",SimTime(),id,avRxFrames/generated_video_frames,controlRTT);

       //double new_load = Load;       
       //if((test_frames_received[id]/generated_video_frames) > 0.95 )
       if(avRxFrames/generated_video_frames > 0.95)
       {
               //if(test_average_delay_decision[id] > (double) 1/fps)
               if(controlRTT > (double) 1/fps)         
               {
                       double p_do_something = (100E6 - new_load)/100E6;
                       //p_do_something = 0.25;
                       if(Random() >= p_do_something)  
                       {               
                               if(traces_on) printf("%f - XR server %d : Decrease Load Delay\n",SimTime(),id);
                               //fps=MAX(30,fps/2);
                               new_load = MAX(10E6,new_load-10E6);
                               load_changes++;
                       }
               }
               else
               {
                       double p_do_something = (100E6 - new_load)/100E6;
                       //p_do_something = 0.25;
                       if(Random() <= p_do_something)  
                       {               
                               if(traces_on) printf("%f - XR server %d : Increase Load Delay\n",SimTime(),id);
                               new_load = MIN(100E6,new_load+10E6);
                               load_changes++;
                       }
                       else
                       {
                               
                               // do nothing to leave room to others
                               double time_next = inter_video_frame.GetTime();

                               inter_video_frame.Cancel();
                               
                               //double update_time = time_next+Random((double)1/fps);
                               double update_time = time_next + Random((double) 1.5/fps);

                               if(traces_on) 
                               printf("%f - XR server %d : Do nothing - Time Next Frame = %f | Updated = %f | Random Values = %f\n",SimTime(),id,time_next,update_time,Random((double) 1.5/fps));

                               inter_video_frame.Set(update_time);
                               
                               
                       }                       

                       //fps=MIN(240,2*fps);
               }
       
       }
       else
       {
                       if(traces_on) printf("%f - XR server : Decrease Load Losses\n",SimTime());
                       //fps=MAX(30,fps/2);
                       new_load = MAX(10E6,new_load-10E6);
                       load_changes++;

       }


       NumberPacketsPerFrame = ceil((new_load/L_data)/fps);
       //tau = (double) L_data/new_load;
       Load = new_load;
       if(traces_on) printf("%f - XR server %d : Time to check fps | New Load = %f (%f - %f - %f) | Losses = %f \n",SimTime(),id,new_load,(double) 1/fps,controlRTT,test_average_delay_decision[id],(test_frames_received[id]/generated_video_frames));


      */
	 #endif

	MAB_rewards[current_action]=(MIN(1,received_frames_MAB/sent_frames_MAB)*Load);    /// UPDATE REWARD OF CURRENT ACTION 
	GreedyControl(t);

	printf("%f - XRserver %d - Reward update %f for current action %d | Received %f and Sent %f\n",SimTime(),id,MAB_rewards[current_action],current_action,received_frames_MAB,sent_frames_MAB);
	
	sent_frames_MAB = 0;
	received_frames_MAB = 0;
	RTT_MAB = 0;
		
	// 2) Next Action
	

	printf("%f - Load = %f | next_action = %d\n",SimTime(),Load,next_action);
	current_action = next_action;

	rate_control.Set(SimTime()+(0.1));
	
};

int XRServer::overuse_detector(double mowdg, double threshold)
{
	if(mowdg > threshold){
		return 2;
	}
	else if( mowdg < (-threshold)){
		return 0;  // UNDERUSE
	}
	else if((-threshold <= mowdg ) && (mowdg<=threshold)){
		return 1;	//NORMAL
	}
	else{
		throw std::runtime_error("THIS IS NOT SUPPOSED TO HAPPEN!!!");
		return 999;
	}
};


#endif
