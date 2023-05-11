/*
	Poisson source. 
*/

#ifndef _XRServer_
#define _XRServer_

#include <vector>
#include <algorithm>	
#include "definitions.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <random>

#include <string>
#include <cstring> // include the cstring header for memcpy

using namespace std;

#define ADAPTIVE_HEUR 	0 		//set to 1 for heuristic adaptive control

#define MAXLOAD 		10E7 	// max load for reward calcs

#define INC_CONTROL 	1.01	//how much we increase or decrease our load depending on action chosen, in Q-LEARNING. 
#define DEC_CONTROL 	0.99

#define N_STATES 		20   	//for feature map of the "Throughput" state space
#define N_ACTIONS_MAB 	10		//For epsilon-greedy MAB approach, where we assume only one state and leverage actions


/* ##############################           AGENT TYPE             #####################################3*/
#define CTL_GREEDY_MAB 	0		// IF SET TO 1, USE MAB INSTEAD OF Q MATRIX
#define CTL_THOMPSON 	0
#define CTL_UCB 	 	0
#define CTL_Q_ONLINE    0

#define TIME_BETWEEN_UPDATES 0.1  //How often the AGENT will choose new ACTION

#define N_ACTIONS_THOMPSON 20 
#define N_ACTIONS_UCB 20 


// Online Q-learning parametres: 
const int ITER_SIZE = 1000;
const int ACTION_SIZE= 3;
const float ALPHA =0.5;
const float GAMMA= 0.9;
const int STATE_SIZE = 10;
double QoE_metric; 
const double alpha_mab = 0.4;


static struct QoE_t{
	double RTT;
	double RXframes;
	double MOWDG; 
	double QoE; 
}QoS_struct; 


component XRServer : public TypeII
{
	
	public: // Default functions
		void Setup();
		void Start();
		void Stop();

		//utilities
		int overuse_detector(double mowdg, double threshold); //function to detect if mowdg is within limits of threshold. 
		
		void GreedyControl();
		void QLearning();
		void ThompsonSampling(); 
		void UpperConfidenceBounds(); 

		void update( int state, int action, double reward, int next_state);

		int feature_map(double Load);
		double reward(int state, int next_a);

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
		
		XRServer () { 
			connect inter_video_frame.to_component,new_video_frame; 
			connect inter_packet_timer.to_component,new_packet;
			connect rate_control.to_component,AdaptiveVideoControl;}

		
	public: // Input parameters
		int id; // sender
		int L_data;
		int destination;
		double Load; // bps
		int current_state; // current state for reward function
		int current_action = 0;
		double fps;
		int node_attached;
		int source_app;
		int destination_app;
		double last_frame_generation_time;
		int rate_control_activated;
		
		struct input_arg_t {
			int seed; 
			double STime;
			int fps; 
			double XRLoad; 
			double BGLoad;
		}st_input_args;

		//Input params end
		int rtt_counter; 
		int next_action;
		int next_action_MAB; 

		double past_load; //just some var to store the last load in order to calculate next one. 

		double rx_f_pl;		//for calculating packet loss over windows of time 
		double sent_f_pl;  

		int state_q;


		double last_mowdg; //aux variable to sample last owdg every 0.1 seconds
		double last_threshold; //aux for last threshold from webrtc value

		double passes; 
		struct csv_t{

			std::vector <double> v__SimTime;	// Time of simulation
			std::vector <double> v__current_action;  //current chosen action from Qlearning
			std::vector <double> v__reward;	//current reward
			std::vector <double> v__load;	//current load
			std::vector <double> v__FM;		//feature map value
			std::vector <double> v__p_p_f; //packets per frame
			std::vector <double> v__QoE; // QoE metric at thatr instant
			std::vector <double> v__frame_loss; // frame loss 
			std::vector <double> v__k_mowdg; //measured one way delay gradient
			std::vector <double> v__RTT; //round trip time
			std::vector <double> v__threshold; 
			std::vector <double> v_quadr_modg; //measure of quadratic sum of measured owdg over window
		}csv_; 
		/////////////////////////////////////////////////////////////////////////////////////
		struct tomp_s_t {
			int current_action;
			double current_reward; 
			double sigma[N_ACTIONS_THOMPSON];
			double action_v[N_ACTIONS_THOMPSON]; //let's try with 20 actions: 5mbps windows
			double n_times_selected[N_ACTIONS_THOMPSON];
			int cntr; 
			std::vector<double> reward_hist; 
			std::vector<double> action_hist;
		}thompson_struct;

		struct ucb_s_t {
			int current_action;
			double current_reward; 
			double sigma[N_ACTIONS_UCB];
			double action_v[N_ACTIONS_UCB]; //let's try with 20 actions: 5mbps windows
			double n_times_selected[N_ACTIONS_UCB];
			double action_confidence[N_ACTIONS_UCB];
			double action_reward[N_ACTIONS_UCB];
			int cntr; 
			std::vector<double> reward_hist; 
			std::vector<double> action_hist;
		}ucb_struct;


		struct sliding_window_t {

			data_packet Packet;
			double      Timestamp; 
			double 		RTT; 
		}; 

		std::vector<sliding_window_t>sliding_vector;  

		double CUMulative_reward; //TODO


		double sequence_frame_counter; //useful for keeping track of frameloss 

		//////////////////////////////////////////////////////////////////////////////
	
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
		
		double MAB_rewards[N_STATES];
		double MAB_rewards_greedy[N_ACTIONS_MAB];
		//int current_action = 0;
		double sent_frames_MAB = 0;
		double received_frames_MAB = 0;
		double RTT_metric = 0;
		double jitter_sum_quadratic = 0; //aux var to keep track of mowdg over windows

		double packet_loss_ratio = 0; 
		double rw_pl;
		double rw_threshold; 

		int signal_overuse; 
		double Q_matrix[N_STATES][3]; 

		double Q_matrix_t10[N_STATES][3]; 
		double Q_matrix_t30[N_STATES][3]; 
		double Q_matrix_t50[N_STATES][3]; 
		double Q_matrix_t100[N_STATES][3]; 	

		double epsilon_greedy_decreasing; 

		/*
		double packet_loss_window;
		double packet_received_window;
		double packet_sent_window; 
		*/

		
		double m_owdg; // measure of filtered delay gradient 

		struct csvfer_t{    ///TODO: CSV OUTPUTS
			double mean_frame_delay_;
			double frame_perc_99_;

			double mean_packet_delay_;
			double packet_delay_99_;
			double ratio_frames_;

			double MAB_r[N_ACTIONS_MAB];
			double sent_frames_MAB = 0;
			double received_frames_MAB = 0;
			double RTT_metric = 0;
		}csv_fer;

		std::vector <csvfer_t> vector_csv;


		//std::vector<std::vector<double>>Q_matrix(10, std::vector<double>(3));
//double MAB_r[N_STATES];

	
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

	printf("seed: %d\n", st_input_args.seed);
    printf("STime: %lf\n", st_input_args.STime);
    printf("fps: %d\n", st_input_args.fps);
    printf("XRLoad: %lf\n", st_input_args.XRLoad);
    printf("BGLoad: %lf\n", st_input_args.BGLoad);

	rx_f_pl = 0;
	sent_f_pl = 0 ;
	state_q = 0 ;
	sequence_frame_counter = 0; 
	
	//values for applying the greedy epsilon_greedy_decreasing
	epsilon_greedy_decreasing = 0.25; 
	passes = 0; 

	#if CTL_GREEDY_MAB ==1
		for (int r=0;r<10;r++)
		{
			MAB_rewards_greedy[r]=0.0;							
			printf("%f ",MAB_rewards_greedy[r]);      // REVIEW: SET T0 1.0 ? 
		}
	#else 

		for (int r=0;r<20;r++)
		{
			MAB_rewards[r]=0.0;
			printf("%f ",MAB_rewards[r]);
		}
	#endif
	thompson_struct.current_action = feature_map(Load) ;//INITIAL LOAD; 
	//thompson_struct.action_hist.push_back( ##INITIAL LOAD ); //First action
	//thompson_struct.reward_hist.push_back( reward( feature_map(Load), 1) );
	//thompson_struct.n_times_selected[thompson_struct.current_action] = 1;
};	
	
void XRServer :: Stop()
{
	printf("------------------ XR Server %d ------------------\n",id);
	printf("Generated Packets = %f | Received Packets = %f\n",generated_packets,received_packets);
	printf("Average RTT (last packet video frame) = %f \n",avRTT/rx_packet_controlRTT);
	printf("Average Load = %f\n",av_Load/generated_video_frames);
	printf("Number of Changes = %f | Rate of changes = %f\n",load_changes,load_changes/SimTime());

	//EXPORT Q MATRICES: 
	ofstream outfile("Results/Qmatrix/Q_fini.txt");
	ofstream outfile1("Results/Qmatrix/Q_t10.txt");
	ofstream outfile2("Results/Qmatrix/Q_t30.txt");
	ofstream outfile3("Results/Qmatrix/Q_t50.txt");
	ofstream outfile4("Results/Qmatrix/Q_t100.txt");

	for (int i = 0; i < N_STATES; ++i) {		// Matrix 1: Q_t10
		for (int j = 0; j < 3; ++j) {
			outfile1 << Q_matrix_t10[i][j] << " ";
		}
		outfile1 << endl;
	}
	outfile1.close();

	for (int i = 0; i < N_STATES; ++i) {	// Matrix 2 Q_t30
		for (int j = 0; j < 3; ++j) {
			outfile2 << Q_matrix_t30[i][j] << "\t";
		}
		outfile2 << endl;
	} 
	outfile2.close();

	for (int i = 0; i < N_STATES; ++i) {	// Matrix 3 Q_t50
		for (int j = 0; j < 3; ++j) {
			outfile3 << Q_matrix_t50[i][j] << "\t";
		}
		outfile3 << endl;
	}
	outfile3.close();

		for (int i = 0; i < N_STATES; ++i) {	// Matrix 4 Q_t100 
			for (int j = 0; j < 3; ++j) {
				outfile4 << Q_matrix_t100[i][j] << "\t";
			}
			outfile4 << endl;
		}
	outfile4.close();
	// 	// Matrix 5 Q_final 
		for (int i = 0; i < N_STATES; ++i) {
			for (int j = 0; j < 3; ++j) {
				outfile << Q_matrix[i][j] << "\t";
			}
			outfile << endl;
		}
	// 
	outfile.close();


	////////////////////   CSV RESULTS ////////////////////////

	std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << (st_input_args.XRLoad/10E6);
    std::string xrl_str = stream.str();

    stream.str("");
    stream << std::fixed << std::setprecision(1) << (st_input_args.BGLoad/10E6);
    std::string bgl_str = stream.str();

	stream.str("");
    stream << std::fixed << std::setprecision(1) << st_input_args.STime;
    std::string stime = stream.str();
	
	#if CTL_GREEDY_MAB ==1
		std::string greedyornot = "MAB-";
	#elif CTL_Q_ONLINE == 1
		std::string greedyornot = "Q-";
	#elif CTL_THOMPSON == 1
		std::string greedyornot = "THMPSN";
	#elif CTL_UCB == 1
		std::string greedyornot = "UCB";
	#else
		std::string greedyornot = "VANILLA";
	#endif

	std::string filename = greedyornot + "Res_T"+ stime +"_FPS"+std::to_string((int)st_input_args.fps) +"_L"+ xrl_str+"_BG"+ bgl_str +".csv";

	//1std::string filename = "Res_T"+std::to_string((int)st_input_args.STime)+"_FPS"+std::to_string((int)st_input_args.fps) +"_L"+std::to_string((int)st_input_args.XRLoad/10E6 )+"_BG"+std::to_string((int)st_input_args.BGLoad/10E6) +".csv";
	printf("\n\nFILENAME: %s\n",filename.c_str());
	std::ofstream file("Results/csv/" + filename);

	if(!file.is_open()){
		std::cout<< "failed to open"<< std::endl;
	}

	file << "simtime,current_action,reward,load,FM_state,packets_per_frame,QoE,frame_loss,last_mowdg,RTT,lastthreshold,quadraticsum_mowdg"<< std::endl;
	for(double i = 0; i < csv_.v__SimTime.size(); i++) //every vector SHOULD be same size
	{
		file << csv_.v__SimTime[i] << "," << csv_.v__current_action[i] << "," << csv_.v__reward[i] << "," << csv_.v__load[i]<<"," << csv_.v__FM[i]	<< "," << csv_.v__p_p_f[i]<< "," << csv_.v__QoE[i]<< "," << csv_.v__frame_loss[i]<<"," << csv_.v__k_mowdg[i]<<"," << csv_.v__RTT[i]<<"," << csv_.v__threshold[i]<<"," << csv_.v_quadr_modg[i]<<	std::endl; 
		//add all metrics to csv output
	}
	file.close();
	#if CTL_GREEDY_MAB ==1
		printf("MAB_REWARDS\n");
		for (int jk = 0;  jk < 10; jk++){
			printf("%d: %f", jk, MAB_rewards_greedy[jk]); 
		}
	#endif
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
	sent_f_pl++;

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
	
};

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
	sequence_frame_counter++; 
	
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
		XR_packet.frame_numseq = sequence_frame_counter; 
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

		/////////////////////////// DEPRECATED  /////////////////
		
		jitter_sum_quadratic += pow(packet.m_owdg, 2); //Quadratic sum of value
		last_mowdg = packet.m_owdg;
		last_threshold = packet.threshold_gamma; 
		int signal_overuse = overuse_detector(packet.m_owdg, packet.threshold_gamma);

		if(signal_overuse == 1){ 
			rw_threshold = 0.5;	//NORMAL IS REWARDED 3 TIMES AS UNDERUSE OR OVERUSE, FOR STABILITY
			//printf("[dbg]NORMAL\n");
		}
		else if (signal_overuse == 2){ 
			//printf("[dbg]OVERUSE\n");
			rw_threshold = 0.3;  //overuse
		}
		else if(signal_overuse == 0){
			rw_threshold = 0.3; //underuse reward
			//printf("[dbg]UNDERUSE\n");

		/////////////////////////// DEPRECATED END /////////////////

		}

		//printf("Quadratic sum_jitter: %f, mowdg: %f, threhsold: %f\n", jitter_sum_quadratic, packet.m_owdg, packet.threshold_gamma); 
	}

	if(packet.last_video_frame_packet == 1)
	{	
		//////////////////// NEW SLIDING WINDOW CODE ///////////////////
	
		sliding_window_t NEW_p; 
		memcpy(&NEW_p.Packet, &packet, sizeof(packet)); 
		NEW_p.Timestamp = SimTime();  
		NEW_p.RTT = NEW_p.Timestamp - packet.TimeSentAtTheServer; 

		sliding_vector.push_back(NEW_p);


		///////////////////// end SLIDING WINDOW CODE ////////////////////
		rx_f_pl++; 
		double RTT = SimTime() - packet.TimeSentAtTheServer;
		avRTT += RTT;
		//if(traces_on) 
		if(traces_on) printf("%f - XR server %d : Uplink Packet received: RTT = %f\n",SimTime(),id,RTT);

		controlRTT = (controlRTT + RTT)/2;
		rx_packet_controlRTT++;

		RTT_metric = (RTT_metric + RTT)/2;   					
												
		avRxFrames = (avRxFrames + packet.frames_received)/2;

		received_frames_MAB++;
		
		//m_owdg = packet.m_owdg; //kalman filter estimate of One Way Delay Gradient!

		//double packet_loss_ratio = received_frames_MAB/sent_frames_MAB;
		packet_loss_ratio = std::abs(rx_f_pl/sent_f_pl);

			
		/*	LEGACY CODE (MIGHT DELETE)

		//printf("Packet loss over 1000 packets \"window\": %f, rw_threshold = %f\n", (1 - packet_loss_ratio), rw_threshold);

		if(sent_f_pl>= 5000 ){
			rx_f_pl = 1 ; 
			sent_f_pl = 1; 
		}
		if(packet_loss_ratio<0.95){ 
			rw_pl = 0;
			}
		else if (packet_loss_ratio > 0.95) {
			rw_pl = Load/MAXLOAD; 				
		}
		*/

	}
	received_packets++;
};

void XRServer :: GreedyControl()
{
	// 2) Next Action
	next_action_MAB= -1;
	if(Random()<=epsilon_greedy_decreasing)
	{
		// Explore
		printf("***************** EXPLORE MAB**************************** %f\n", epsilon_greedy_decreasing);
		next_action_MAB = Random(10); //Choose between 10,20,30,40,50,60,70,80,90 or 100 MBps 
		/*
		//If action = 0: increase. 	
		if a == 1: KEEP load, 
		if a == 2: Decrease Load */
	}
	else
	{
		printf("***************** EXPLOIT MAB **************************** %f\n", epsilon_greedy_decreasing);

		// Get the maximum 
		int index_max = 0;
		double max_reward = MAB_rewards_greedy[0];
		for (int r=0;r<10;r++)
		{
			//printf("%d %f\n",r,MAB_rewards[r]);
			if(max_reward < MAB_rewards_greedy[r])
			{
				index_max = r;
				max_reward = MAB_rewards_greedy[r];
			}
		}
		printf("[E-GREEDY] The action with max reward is %d\n",index_max);
		next_action_MAB = index_max;
	}

	Load = (next_action_MAB + 1) * 10E6; 
	NumberPacketsPerFrame = ceil((Load/L_data)/fps);

	printf("%f - Load = %.1fE6 | next_action = %d\n",SimTime(),Load/(10E6),next_action_MAB);
	
	current_action = next_action_MAB;
	
	//update reward
	MAB_rewards_greedy[current_action]= alpha_mab * MAB_rewards_greedy[current_action] + (1-alpha_mab)*(90*MIN(1,received_frames_MAB/sent_frames_MAB)+10*(Load/10E7))/100;
	//update ε
	epsilon_greedy_decreasing = MAX(0.1, (0.25 - passes / 20000.0 )) ; //update "epsilon threshold" to decrease exploration linearly after some time, limited at 0.1
	
	
};

// Define the update function
void XRServer :: update( int state, int action, double reward, int next_state) {
    double old_value = Q_matrix[state][action];
    //double next_max = *max_element(Q[next_state].begin(), Q[next_state].end());
	double next_max = *max_element(std::begin(Q_matrix[next_state]),  std::end(Q_matrix[next_state]));
    double new_value = (1 - ALPHA) * old_value + ALPHA * (reward + GAMMA * next_max);

    Q_matrix[state][action] = new_value;
};
//void reward(int a, int b){}


void XRServer :: ThompsonSampling()
{
	if (passes == 1) {
		//printf("First time algorithm has ran, however metrics should still be available from sliding window\n\n");
		//WORKS, SO: first time going through here we don't update rewards, but next yes!!!
		//TODO: need to keep track of past action!!
	}
	else{
		//double reward_past_action = QoS_struct // TODO: COMPLETE
		//rw = 0.95* QoE_metric + 0.05*( (feature_map(past_load) * 5E6)/MAXLOAD);

		//upload reward based on QoE_metric

	}
	past_load = Load; 
	std::random_device rd;
	std::mt19937 gen(rd());
	std::normal_distribution<> d(0, 1); //code for randn approach in matlab

	for (int i= 0; i<=N_ACTIONS_THOMPSON;i++) //sample from gaussian distribution with nº of times action k has been taken
	{
		double sample = thompson_struct.action_v[i];
		thompson_struct.sigma[i] = 1/(thompson_struct.n_times_selected[i] + 1);
		thompson_struct.action_v[i] = sample + thompson_struct.sigma[i] * d(gen); //theta
		
	}

	//find argmax of the possible actions sampled from vector
	
	int argmax = 0; 
	double max_val = thompson_struct.action_v[0];

	for(int i = 1; i<N_ACTIONS_THOMPSON; i++){
		if(thompson_struct.action_v[i]>max_val){
			max_val = thompson_struct.action_v[i];
			argmax = i; 
		}
	}

	thompson_struct.current_action = argmax;
	thompson_struct.n_times_selected[argmax]++; 
	printf("THOMPSON: action taken: %d, n_times of action: %f", argmax, thompson_struct.n_times_selected[argmax]);
		
	Load = thompson_struct.current_action * 5E6; 

	thompson_struct.current_reward = reward(thompson_struct.current_action, 1); //"1" because it's same reward function from q-learning adapted "without state-action" 
	thompson_struct.reward_hist.push_back(thompson_struct.current_reward);
	thompson_struct.action_hist.push_back(thompson_struct.current_action);

	current_action = thompson_struct.current_action; 
	thompson_struct.action_v[argmax] = thompson_struct.action_v[argmax] * ( thompson_struct.n_times_selected[argmax] - 1 ) + thompson_struct.current_reward/(thompson_struct.n_times_selected[argmax]); //update value action matrix 
};

void XRServer::UpperConfidenceBounds()
{
	past_load = Load; 

	for (int i = 0; i< N_ACTIONS_UCB; i++) 
	{
		double sample = ucb_struct.action_v[i] + sqrt(( 2 * log(ucb_struct.cntr)) / ucb_struct.n_times_selected[i]);
		ucb_struct.action_confidence[i] = sample; 
	}
	int argmax = 0;
	double max_val = ucb_struct.action_confidence[0];

	for( int i = 1; i<N_ACTIONS_UCB; i++ )
	{
		if(ucb_struct.action_confidence[i]>max_val){
			max_val = ucb_struct.action_confidence[i];
			argmax = i; 
		}
	}
	ucb_struct.current_action = argmax;
	ucb_struct.n_times_selected[argmax] ++; 

	Load = ucb_struct.current_action * 5E6; 
	printf("UCB: Action taken %d , nº times of action: %f", argmax, ucb_struct.n_times_selected[argmax]);

	ucb_struct.action_reward[argmax] += reward(argmax, 1); 

	current_action = ucb_struct.current_action; 

	ucb_struct.action_v[argmax] = ucb_struct.action_reward[argmax] / ucb_struct.n_times_selected[argmax]; 
	ucb_struct.cntr++ ;
};

void XRServer :: QLearning()
{
        // Reset the current state to 0        TEST: NOT RECURSIVE MAYBE
        //int state_q = 0;
		//next_action = 0;

        // Loop over steps in the episode
        // Choose an action using an epsilon-greedy policy
            //double epsilon = 0.1;

			past_load = Load; 

            if(Random()<= 0.25) //TODO: ADD linear decreasing epsilon BASED ON KNOWLEDGE/BELIEF (OF CURRENT STATE? ) 
			{	// Explore
				printf("***************** EXPLORE Q **************************** %f\n", SimTime());
				next_action = Random(2);

						//If action = 0: decrease. 	
						//if a == 1: KEEP load, 
						//if a == 2: Increase Load */
			} 	
			else
			{
				printf("***************** EXPLOIT Q**************************** %f\n", SimTime());

                // Choose the action with the highest Q-value
				for (int a = 0; a < ACTION_SIZE; a++)
				{
                	next_action = Q_matrix[state_q][a] > Q_matrix[state_q][current_action] ? a : current_action;
				}
            }
			printf("Next action: %d\n ", next_action);
			if(next_action == 0){				//CHOOSE NEXT LOAD BASED ON ACTION
				Load = DEC_CONTROL * past_load; 
				load_changes++;
			}
			else if (next_action == 1){ //keep
				Load = past_load;
			}
			else if (next_action == 2){ //increase
				Load = INC_CONTROL * past_load;
				load_changes++; 
				if(Load >= MAXLOAD){
					Load = MAXLOAD;
					printf("WARN : In maxload already!\n");
				}
			}

			printf("LOAD IS: %f \n", Load);

			int next_state = feature_map(Load); 
			
			NumberPacketsPerFrame = ceil((Load/L_data)/fps);	

            // Calculate the reward and next state
            double r = reward(state_q, next_action);
            // Update the Q-value for the current state-action pair
            		
			update(state_q, next_action, r, next_state);

            // Update the current state
			current_action = next_action;
			current_state = next_state; //
			state_q = next_state; 			// WARNING CHECK AND TEST THIS
            printf("next state: %d", next_state);
							
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

//// 1: Apply "one pass" of algorithm

	//TEST: GET METRICS OVER "SLIDING WINDOW"

	printf("iterating through sliding vector:\n");

	double CurrentTime = SimTime(); 
	double RTT_slide = 0;
	double RX_frames_slide = 0; 
	double MOWDG_slide = 0;
	double Frameloss_slide = 0; 
	double no_lost_packets = 0; 


	int no_packets = 0;
	int no_feedback_packets = 0;  
 
	for (auto it = sliding_vector.begin(); it!=sliding_vector.end(); ){
		if(it->Timestamp <= (CurrentTime - TIME_BETWEEN_UPDATES))
		{
			it = sliding_vector.erase(it);
		}
		else{
			it ++;
			no_packets ++; 
			//Calculate metrics over the rest of vector: 
			RTT_slide += it->RTT; 
			RX_frames_slide += it->Packet.frames_received;
			
			if(it->Packet.feedback == true){
				MOWDG_slide += it->Packet.m_owdg; 
				no_feedback_packets++; 
			}
		}

	}
	
	//And compute averages over this window
	if((no_feedback_packets != 0) && (no_packets != 0) ){		//just make sure to not divide by 0


		//CALCULATE FRAME LOSS OVER WINDOW: 
			for(int i = 1; i < no_packets; i++){
				int diff = sliding_vector[i].Packet.frame_numseq - sliding_vector[i-1].Packet.frame_numseq;
				if (diff>1){
					no_lost_packets++; 
				}
			}				

		RTT_slide = RTT_slide / no_packets;
		RX_frames_slide = RX_frames_slide / no_packets ;
		MOWDG_slide = MOWDG_slide/ no_feedback_packets; 
		Frameloss_slide = no_lost_packets/(no_packets + no_lost_packets);

		printf("\n\n[DBG Metrics] Sliding window:\nRTT:.%.4f\tRX_frames: %.2f\t MOWDG:%.4f,\tFrameloss: %.3f\n", RTT_slide, RX_frames_slide, MOWDG_slide, Frameloss_slide);
		
		QoS_struct.RTT = RTT_slide; 
		QoS_struct.RXframes = RX_frames_slide; //divide by sent packets
		QoS_struct.MOWDG = MOWDG_slide; 

		//FER REWARDS
		//QoE_metric = 3.01 * exp(-4.473 * (1-packet_loss_ratio)) + 1.065; // metric only taking into account the packet loss ratio

		//QoE_metric = 3.01 * exp( -4.473 * (0.8 * (1 - packet_loss_ratio)*10E2 + 0.2*jitter_sum_quadratic)*10E2) + 1.065; // metric with webrtc congestion control added on top of packet loss
		
		//QoE_metric = QoE_metric/4.075 ; // NORMALIZE QOE TO 1? 
		//double QoE_metric2 = 3.01 * exp( -4.473 * (0.33 * packet_loss_ratio + 0.33 * rw_threshold + 0.34 * (1- jitter_sum_quadratic) )) + 1.065; // metric with webrtc congestion control added on top of packet loss + a reward for less jittery outcomes. 
		
		//BORIS REWARDS
		QoE_metric = ((1/fps)/RTT_metric) *(rw_pl) ;			//metric proposed by boris to leverage different metrics
		//double instantaneous_reward_Boris = (90*MIN(1,received_frames_MAB/sent_frames_MAB)+10*(Load/100E6))/100;	// second reward function proposed by Boris

		//printf("QOE normalized: %f\n\n", QoE_metric);

		/*
		if ( QoE_metric < )
		QoE_rw += QoE_metric
		*/
		  //let the jitter_sum_quadratic go back to 0 for next frame measurement
	}
	else{
		printf("dividing by zero????????????????????\n");

		// make reward go to 0 in this case [Boris suggestion]
	}
	//END TEST
	passes++; 

	//printf("passes: %f, TIME: %f\n", passes, SimTime()); 
	
	#if CTL_THOMPSON == 1
		ThompsonSampling();
		// maybe need to add reward vector? TODO 
 
	#elif CTL_UCB == 1
		UpperConfidenceBounds(); 
		// maybe need to add reward vector? 
	#elif CTL_GREEDY_MAB == 1
		GreedyControl();

	#elif CTL_Q_ONLINE == 1 
		QLearning(); 
	#endif

	printf("%f - XRserver %d - Reward update %f for current action %d | Received %f and Sent %f\n",SimTime(),id,MAB_rewards_greedy[current_action],current_action,received_frames_MAB,sent_frames_MAB);
			
// 2) Next Action done, store in CSV 
	//printf("%f - Load = %f | next_action = %d\n",SimTime(),Load,next_action);

	#if CTL_GREEDY_MAB == 1 //if MAB approach
		
		double t___ = SimTime();
		csv_.v__SimTime.push_back(t___);
		csv_.v__current_action.push_back(current_action);
		csv_.v__reward.push_back(reward(current_state, current_action));
		csv_.v__load.push_back(Load);
		csv_.v__FM.push_back(feature_map(Load));
		csv_.v__QoE.push_back(QoE_metric);
		csv_.v__p_p_f.push_back(NumberPacketsPerFrame);
		csv_.v__frame_loss.push_back((1-packet_loss_ratio));
		csv_.v__k_mowdg.push_back(last_mowdg);
		csv_.v__threshold.push_back(last_threshold);
		csv_.v__RTT.push_back(RTT_metric);
		csv_.v_quadr_modg.push_back(jitter_sum_quadratic);
			
	#elif CTL_Q_ONLINE ==  1 //if Q-learning approach apply this control
	// UPDATE ALL CSV VECTORS
		double t___ = SimTime();
		csv_.v__SimTime.push_back(t___);
		csv_.v__current_action.push_back(current_action);
		csv_.v__reward.push_back(reward(current_state, current_action));
		csv_.v__load.push_back(Load);
		csv_.v__FM.push_back(feature_map(Load));
		csv_.v__QoE.push_back(QoE_metric);
		csv_.v__p_p_f.push_back(NumberPacketsPerFrame);
		csv_.v__frame_loss.push_back((1-packet_loss_ratio));
		csv_.v__k_mowdg.push_back(last_mowdg);
		csv_.v__threshold.push_back(last_threshold);
		csv_.v__RTT.push_back(RTT_metric);
		csv_.v_quadr_modg.push_back(jitter_sum_quadratic);
		
		if(passes == 100) //10 seconds
		{
			std::memcpy(Q_matrix_t10, Q_matrix, sizeof(Q_matrix)); 
		}
		else if(passes == 300) //30 seconds
		{
			std::memcpy(Q_matrix_t30, Q_matrix, sizeof(Q_matrix)); 
		}
		else if(passes == 500) //50 seconds
		{
			std::memcpy(Q_matrix_t50, Q_matrix, sizeof(Q_matrix)); 
		}
		else if(passes == 1000) //100 seconds
		{
			std::memcpy(Q_matrix_t100, Q_matrix, sizeof(Q_matrix)); 
		}

		sent_frames_MAB = 0;
		received_frames_MAB = 0;
		RTT_metric = 0;
		jitter_sum_quadratic = 0;
	#elif CTL_UCB == 1
		double t___ = SimTime();
		csv_.v__SimTime.push_back(t___);
		csv_.v__current_action.push_back(ucb_struct.current_action);
		csv_.v__reward.push_back(reward(2, current_action)); // arbitrary int "2" to get reward not based on state
		csv_.v__load.push_back(Load);
		csv_.v__FM.push_back(feature_map(Load));
		csv_.v__QoE.push_back(QoE_metric);
		csv_.v__p_p_f.push_back(NumberPacketsPerFrame);
		csv_.v__frame_loss.push_back((1-packet_loss_ratio));
		csv_.v__k_mowdg.push_back(last_mowdg);
		csv_.v__threshold.push_back(last_threshold);
		csv_.v__RTT.push_back(RTT_metric);
		csv_.v_quadr_modg.push_back(jitter_sum_quadratic);

	#elif CTL_THOMPSON == 1 

		double t___ = SimTime();
		csv_.v__SimTime.push_back(t___);
		csv_.v__current_action.push_back(current_action);
		csv_.v__reward.push_back(reward(2, current_action)); // arbitrary int "2" to get reward not based on state
		csv_.v__load.push_back(Load);
		csv_.v__FM.push_back(feature_map(Load));
		csv_.v__QoE.push_back(QoE_metric);
		csv_.v__p_p_f.push_back(NumberPacketsPerFrame);
		csv_.v__frame_loss.push_back((1-packet_loss_ratio));
		csv_.v__k_mowdg.push_back(last_mowdg);
		csv_.v__threshold.push_back(last_threshold);
		csv_.v__RTT.push_back(RTT_metric);
		csv_.v_quadr_modg.push_back(jitter_sum_quadratic);

	#endif

	rate_control.Set(SimTime()+(TIME_BETWEEN_UPDATES));  //RATE CONTROL EVERY 0.1 SECONDS


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

/* //ARRAY VERSION
int* XRServer::feature_map(double Load){

	static int State[21]; 
	for (int i = 0; i < 21; ++i){ //iterate through state vector
        
		if ((Load >= 5E6*(i-1)) && (Load <= (5E6 *i -1))){
		    printf("load between %f, %f", 5E6*(i-1), (5E6 *i -1));
			State[i] = 1;
            }
	}
    return State;
}
*/

int XRServer::feature_map(double Load){

	static int State; 
	for (int i = 0; i < 21; ++i){ //iterate through whole state vector
		if ((Load >= 5E6*(i-1)) && (Load <= (5E6 *i -1))){
		    //printf("load between %f, %f", 5E6*(i-1), (5E6 *i -1));
			State = i;
            }
	}
    return State;
};


double XRServer::reward(int state, int next_a){
	
	double rw;
	if(state == 1){
		if (next_a == 0) 
		{
			rw = -1; 
		}
	}
	else if (state == 20){
		if (next_a == 2 ){
			rw = -1;	//let's try to stay far from edges 
		}
	} 
	
	else {rw = 0.95* QoE_metric + 0.05*( (state * 5E6)/MAXLOAD);}
	printf("\n\n\treward: %.2f\t QoE metric: %.3f\n", rw, QoE_metric);
	return rw; 
};

/*
void updateState(int signal) {
    switch (currentState) {
        case HOLD:
            if (signal == underuse_S) {
                currentState = HOLD;
            } 
            else if(signal == normal_S) {
                currentState = INCR;
                // remain in current state
            }
            else if(signal == overuse_S){
                currentState = DECR;
            }
            else{printf("????????????");}
            break;
        case DECR:
            if (signal == underuse_S) {
                currentState = HOLD;
            } 
            else if(signal == normal_S) {
                currentState = HOLD;
                // remain in current state
            }
            else if(signal == overuse_S){
                currentState = DECR;
            }
            else{printf("????");}
            break;
			
        case INCR:
			if (signal == underuse_S) {
                currentState = HOLD;
            } 
            else if(signal == normal_S) {
                currentState = INCR;
                // remain in current state
            }
            else if(signal == overuse_S){
                currentState = DECR;
            }
            else{
				printf("?");
			}
            break;     
    }
};
*/




#endif
