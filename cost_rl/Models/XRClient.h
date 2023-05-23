/*
	Poisson source. 
*/

#ifndef _XRClient_
#define _XRClient_
		
#include "definitions.h"
#include <algorithm>
#include <numeric>

#define Q_NOISE 10E-1 // (can) PLAY WITH THIS PARAMETER, IT'S STATE NOISE (for kalman filter) 

//#define N_FRAMES_WINDOW 450 //90 FPS --> 5 SECONDS
#define TIME_SLIDING_WINDOW_CLIENT  1.0  //How many packets are temporally taken into account for sliding window. NOW: 1 second
#define DBG_CLIENT 0

component XRClient : public TypeII
{
	
	public: // Default functions
		void Setup();
		void Start();
		void Stop();

		//"utilities"
		double K_gamma(double mowdf, double gamma_prev); 

	public: // Connections
		outport void out(data_packet &packet);
		inport void in(data_packet &packet);	

		// Timer
		Timer <trigger_t> inter_packet_timer;
		Timer <trigger_t> sliding_window_timer;
		inport inline void new_packet(trigger_t& t); // action that takes place when timer expires
		inport inline void sliding_window_routine(trigger_t& t2); 

		XRClient () { connect inter_packet_timer.to_component,new_packet;
		connect sliding_window_timer.to_component,sliding_window_routine;
		 }


	public: // Input parameters
		int L_data;
		int id; // sender
		int destination;
		double Load; // bps
		int node_attached;
		int source_app;
		int destination_app;
		int fps;

	 	std::vector<double> packet_times;
		std::vector<double> frame_times;

		double Kalman_measured_delay; 
		double countt; 
		double abs_m; 

		struct Kalman_t{
			double t_prev, T_prev;
			double t_current, T_current;
			double OW_Delay;

			double K_gain;
			double m_current; 
			double m_prev; 
			double residual_z;

			double noise_estimation; 
			double noise_prev; 
			double P_current; 
			double P_prev;
			std::vector <double> v_OWDG; // "the good measure"
			std::vector <double> v_jitter; //noisy measured jitter
			std::vector <double> v_Kalman;	//measured gain
			std::vector <double> v_simTime; //simtime
		}Kalman; 

		struct Threshold_t{
			double gamma;
			double gamma_prev;
			double dT;
			double abs_OWDG;
			double K_gamma;
		}Threshold;

		std::vector <sliding_window_t> sliding_vector_client;  
	
		struct sliding_routine_t {
		

		}; //TODO 


	private:
		double tau; //

	public:
		double received_packets=0;
		double generated_packets=0;
		double avDelay=0;
		double avRxPacketSize=0;
		double probFrameLost = 0;
		double received_frames = 0;
		int received_packets_in_current_frame = 0;
		double packets_rx_video_frames[100000]={0};
		double VideoFramesReceived=0;
		double VideoFramesFullReceived=0;

		double mean_VFD = 0;
		double p99th_VFD = 0;
		
		double SEQNUMMAX; 
};

void XRClient :: Setup()
{
	printf("XR Client Setup()\n");
};

void XRClient :: Start()
{
	// We need to map these values to the uplink feedback + statistics
	printf("%f - XR Client Starts() Load = %f | L= %d\n",SimTime(),Load,L_data);

	tau = (double) 1/(2*fps);
	printf("%f\n",tau);
	inter_packet_timer.Set(SimTime()+Exponential(tau));
	//sliding_window_timer.Set(SimTime()+0.5);		//TEST_ NOT CALLING ROUTINE (SEGFAULT)



	//KALMAN FILTER FOR OWDG: INTIALIZE
	Kalman.T_prev= 0;
	Kalman.t_prev = 0; 

	Kalman.m_current = 0; 
	Kalman.P_current = 0.1; //set to 10^⁻1 for P(0) 
	Kalman.noise_prev = 0; 
	Kalman.residual_z = 0; 
	Kalman.noise_estimation= 0; 
	Kalman.P_prev = 0; 
	Kalman.K_gain = 0; 

	Kalman_measured_delay = 0; 
	countt = 0;
	Threshold.gamma = 0; 
	Threshold.gamma_prev = 0; 
	received_packets_in_current_frame = 0; 
 
};
	
void XRClient :: Stop()
{
	printf("-------------- XR Client %d Results --------------\n",id);
	//printf("Generated Packets = %f | Received Packets = %f\n",generated_packets,received_packets);
	printf("Video Thoughput = %f\n",avRxPacketSize/SimTime());	
	//printf("Average Packet Delay = %f\n",avDelay/received_packets);
	//printf("Probability to lose a frame = %f (%f / %f)\n",probFrameLost/received_frames, probFrameLost,received_frames);
	printf("Number of Video Frames Full Received %f from Total = %f | Fraction = %f\n",VideoFramesFullReceived,VideoFramesReceived,VideoFramesFullReceived/VideoFramesReceived);

	std::sort(packet_times.begin(),packet_times.end()); // sort dels valors en el vector
	double avg = std::accumulate(packet_times.begin(),packet_times.end(),0.0)/(double)packet_times.size(); // mitjana per si la voleu, cal #include <numeric>

	// els percentils que volgueu, aquí teniu 1,50,95, 99,  99.9999 i 100
	double perc_1 = packet_times[ceil(0.01*packet_times.size()-1)]; 
	double perc_50 = packet_times[ceil(0.50*packet_times.size()-1)];
	double perc_95 = packet_times[ceil(0.95*packet_times.size()-1)];
	double perc_99 = packet_times[ceil(0.99*packet_times.size()-1)];
	double perc_999999 = packet_times[ceil(0.999999*packet_times.size()-1)];
	double perc_100 = packet_times[(packet_times.size()-1)];

	printf("Video Packets --------------------------------------------------------\n");
	printf(" 1%%-tile:  median:  avg:  95%%-tile:  99%%-tile: 99.9999%%-tile: max: %f %f %f %f %f %f %f\n",perc_1,perc_50,avg,perc_95,perc_99,perc_999999,perc_100);
	
	if(VideoFramesFullReceived > 0)
	{
	// Video Frame delay
	std::sort(frame_times.begin(),frame_times.end()); // sort dels valors en el vector
	double frame_avg = std::accumulate(frame_times.begin(),frame_times.end(),0.0)/(double)frame_times.size(); // mitjana per si la voleu, cal #include <numeric>

	// els percentils que volgueu, aquí teniu 1,50,95, 99,  99.9999 i 100
	double frame_perc_1 = frame_times[ceil(0.01*frame_times.size()-1)]; 
	double frame_perc_50 = frame_times[ceil(0.50*frame_times.size()-1)];
	double frame_perc_95 = frame_times[ceil(0.95*frame_times.size()-1)];
	double frame_perc_99 = frame_times[ceil(0.99*frame_times.size()-1)];
	double frame_perc_999999 = frame_times[ceil(0.999999*frame_times.size()-1)];
	double frame_perc_100 = frame_times[(frame_times.size()-1)];

	printf("Video Frames --------------------------------------------------------\n");
	printf(" 1%%-tile:  median:  avg:  95%%-tile:  99%%-tile: 99.9999%%-tile: max: %f %f %f %f %f %f %f\n",frame_perc_1,frame_perc_50,frame_avg,frame_perc_95,frame_perc_99,frame_perc_999999,frame_perc_100);

	mean_VFD = frame_avg;
	p99th_VFD = frame_perc_99;

	}
}; 


void XRClient :: new_packet(trigger_t &)
{
	if(traces_on) printf("%f - XRClient %d : Uplink Packet generated (tau = %f)\n",SimTime(),id,tau);

	generated_packets++; 

	/*
	data_packet test_packet;
	test_packet.L_data = L_data;
	test_packet.L_data = 100;
	test_packet.L = 100 + L_data;

	test_packet.source = node_attached;
	test_packet.destination = destination;
	test_packet.source_app = source_app;
	test_packet.destination_app = destination_app;
	*/

	data_packet XR_packet;
	XR_packet.L = 20*8 + L_data; // in the sim
	XR_packet.source = node_attached;
	XR_packet.destination = destination;
	XR_packet.source_app = source_app;
	XR_packet.destination_app = destination_app;
	XR_packet.sent_time = SimTime();
	XR_packet.last_video_frame_packet=0;
	XR_packet.frame_numseq = SEQNUMMAX;
	// To compute RTT;
	XR_packet.TimeSentAtTheServer = 0; // No interactive traffic, so no way to compute the RTT
	XR_packet.TimeReceivedAtTheClient = SimTime();
	out(XR_packet);

	inter_packet_timer.Set(SimTime()+tau);	

};

void XRClient :: in(data_packet &packet)
{
	if(traces_on) printf("%f - XRClient %d. Downlink Data Received %d (last packet video frame? %d) From video frame %f <-----------\n",SimTime(),id,packet.num_packet_in_the_frame,packet.last_video_frame_packet,packet.video_frame_seq);
	received_packets++;

	received_packets_in_current_frame++; 
	avDelay += SimTime()-packet.sent_time;
	packet_times.push_back(SimTime()-packet.sent_time);
	avRxPacketSize +=packet.L_data;

	int FullVideoFrameRX = 0; 


	// Probability of not receiving a video frame completely
	//printf("Packet in the frame = %d\n",packet.num_packet_in_the_frame);

	/* // I DO THE SAME THING IN THE LAST PACKET FROM FRAME. 
	if(packet.rtt==true){
			//set packet function to trigger in 50E-3 seconds, however queue may be implemented with stable rate (fps)
			
			countt +=1; 
			
			//KALMAN

			Kalman.t_current = SimTime();
			Kalman.T_current = packet.send_time;

			printf("\tt-1: %f, t: %f, T-1: %f, T: %f\n", Kalman.t_prev, Kalman.t_current, Kalman.T_prev, Kalman.T_current);
			
			Kalman.OW_Delay = (Kalman.t_current - Kalman.t_prev) - (Kalman.T_current - Kalman.T_prev); //measured OW delay (d_m)

			Kalman.K_gain = (Kalman.P_prev +Q_NOISE) /( Kalman.P_prev + Q_NOISE + Kalman.noise_estimation); 
			
			Kalman.m_current = (1-Kalman.K_gain)*Kalman.m_prev + Kalman.K_gain *Kalman.OW_Delay; 
			
			Kalman.residual_z = Kalman.OW_Delay - Kalman.m_prev;
			
			Kalman.noise_estimation = (0.95 * Kalman.noise_prev) +pow(Kalman.residual_z,2)*0.05; // Estimation of the measurement noise variance : sigma_n² (Beta = 0.95) 
			
			Kalman.P_current = (1-Kalman.K_gain)*(Kalman.P_prev + Q_NOISE); //system error variance = Expected value of (avg_m - m(ti))²

			printf("One way delay(10 packets): %f, K_gain: %f, MEASURED DELAY %f\n\n", Kalman.OW_Delay, Kalman.K_gain, Kalman.m_current);
			
			//Add all stats to vector for every instance, for posterior analysis
			Kalman.v_OWDG.push_back(Kalman.m_current);
			Kalman.v_jitter.push_back(Kalman.OW_Delay);
			Kalman.v_Kalman.push_back(Kalman.K_gain);
			Kalman.v_simTime.push_back(SimTime());	

			//update variables for (t-1) in next execution
			Kalman.T_prev = Kalman.T_current; 
			Kalman.t_prev = Kalman.t_current;

			Kalman.P_prev = Kalman.P_current; 
			Kalman.m_prev = Kalman.m_current;
			Kalman.noise_prev = Kalman.noise_estimation; 

			Kalman_measured_delay += Kalman.m_current; 

			packet.feedback = true; 
			packet.m_owdg = Kalman.m_current; //we set the packet's Kalman stats
			//abs_m = std::fabs(Kalman.m_current); this was for threshold  unneeded
		}
	else{packet.feedback = false; }

			//KALMAN FILTER END */

	if(packet.video_frame_seq < 100000) { //DELETE THIS
	if(packets_rx_video_frames[(int) packet.video_frame_seq] == 0) VideoFramesReceived++;
	
	packets_rx_video_frames[(int) packet.video_frame_seq]++;
	if(packets_rx_video_frames[(int) packet.video_frame_seq] == packet.NumPacketsPerFrame)
	{	
		VideoFramesFullReceived++;
		FullVideoFrameRX = 1; 
		frame_times.push_back(SimTime()-packet.frame_generation_time);
		if(traces_on) printf("%f - XRClient %d. Video Frame Received  %f (Packets = %f | Packets In Frame = %d).\n",SimTime(),id,packet.video_frame_seq,packets_rx_video_frames[(int) packet.video_frame_seq],packet.NumPacketsPerFrame);
		if(traces_on) printf("Number of Video Frames Full Received %f from Total = %f | Fraction = %f\n",VideoFramesFullReceived,VideoFramesReceived,VideoFramesFullReceived/VideoFramesReceived);

		test_frames_received[id]++;
		// Video Frame Delay
		test_average_delay_decision[id] = (test_average_delay_decision[id] + (SimTime()-packet.frame_generation_time))/2;
		}
	}




	// Packet Delay
	//test_average_delay_decision[id] = (test_average_delay_decision[id] + (SimTime()-packet.sent_time))/2;

	
	/*
	// Old code ----------
	
	if(packet.first_video_frame_packet == 1) received_packets_in_current_frame = 1;
	else received_packets_in_current_frame++;	

	if(packet.last_video_frame_packet == 1) 
	{
		received_frames++;
		if(received_packets_in_current_frame == packet.NumPacketsPerFrame)
		{	
			printf("%f - XRClient %d. Video Frame Received  %f (Packets = %d | Packets In Frame = %d).\n",SimTime(),id,packet.video_frame_seq,received_packets_in_current_frame,packet.NumPacketsPerFrame);
		}
		else
		{
			printf("%f - XRClient %d. Video Frame Lost.\n",SimTime(),id);
			probFrameLost++;
		}
	}
		// For interactive (correlated traffic: Last packet, or random

	*/

	if(packet.last_video_frame_packet==1){

		SEQNUMMAX = packet.frame_numseq; //just copy the last sequence number in here
		//printf("RECEIVED SEQNUMMAX: %.1f\n", packet.frame_numseq);
		
		
		////////////////// SLIDING WINDOW CLIENT /////////////////////
		sliding_window_t NEW_p; 
		memcpy(&NEW_p.Packet, &packet, sizeof(data_packet)); 
		NEW_p.Timestamp = SimTime();  
		NEW_p.RTT = NEW_p.Timestamp - packet.TimeSentAtTheServer;

		//NEW_p.num_seq = packet.frame_numseq; 
		memcpy(&NEW_p.num_seq, &packet.frame_numseq, sizeof(double)); //TEST

		sliding_vector_client.push_back(NEW_p);
		
		#if DBG_CLIENT
		printf("\t[DBG_CLIENT]: numseq packet: %.1f, struct: %.1f\n", packet.frame_numseq, NEW_p.num_seq);
		#endif
		////////////////// SLIDING WINDOW CLIENT END  /////////////////
		
	}

	
	if(FullVideoFrameRX == 1 )
	{
		if(traces_on) {
			printf("%f - XR client %d . UL packet for RTT\n",SimTime(),id);
			}
		data_packet XR_packet = packet;
		XR_packet.L = 20*8 + L_data;
		XR_packet.source = node_attached;
		XR_packet.destination = destination;
		XR_packet.source_app = source_app;
		XR_packet.destination_app = destination_app;
		XR_packet.sent_time = SimTime();
		// To compute RTT;
		XR_packet.TimeSentAtTheServer = packet.frame_generation_time;
		XR_packet.TimeReceivedAtTheClient = SimTime();
		XR_packet.frames_received = VideoFramesFullReceived;

		XR_packet.packets_received = received_packets; 

		///KALMAN STUFF : //SHOULD WE DO THIS FOR EVERY N PACKETS OR LAST PACKET OF FRAME? 
		countt +=1; 
		Kalman.t_current = SimTime();
		Kalman.T_current = packet.sent_time;

		//printf("\tt-1: %f, t: %f, T-1: %f, T: %f\n", Kalman.t_prev, Kalman.t_current, Kalman.T_prev, Kalman.T_current);
		Kalman.OW_Delay = (Kalman.t_current - Kalman.t_prev) - (Kalman.T_current - Kalman.T_prev); //measured OW delay (d_m)
		Kalman.K_gain = (Kalman.P_prev +Q_NOISE) /( Kalman.P_prev + Q_NOISE + Kalman.noise_estimation); 
		Kalman.m_current = (1-Kalman.K_gain)*Kalman.m_prev + Kalman.K_gain *Kalman.OW_Delay; 
		Kalman.residual_z = Kalman.OW_Delay - Kalman.m_prev;	
		Kalman.noise_estimation = (0.95 * Kalman.noise_prev) +pow(Kalman.residual_z,2)*0.05; // Estimation of the measurement noise variance : sigma_n² (Beta = 0.95) 
	
		Kalman.P_current = (1-Kalman.K_gain)*(Kalman.P_prev + Q_NOISE); //system error variance = Expected value of (avg_m - m(ti))²

		//printf("One way delay(10 packets): %f, K_gain: %f, MEASURED DELAY %f\n\n", Kalman.OW_Delay, Kalman.K_gain, Kalman.m_current);
		abs_m = std::fabs(Kalman.m_current);

		//KALMAN FILTER END

		//THRESHOLD CALCULATION
		Threshold.gamma = Threshold.gamma_prev + (Kalman.t_current - Kalman.t_prev) * K_gamma( Kalman.OW_Delay, Threshold.gamma_prev) * (abs_m - Threshold.gamma_prev);

		//printf("Threshold gamma: %f, Threshold(t-1): %f ", Threshold.gamma, Threshold.gamma_prev);

		//Add all stats to vector for every instance, for posterior analysis
		Kalman.v_OWDG.push_back(Kalman.m_current);
		Kalman.v_jitter.push_back(Kalman.OW_Delay);
		Kalman.v_Kalman.push_back(Kalman.K_gain);
		Kalman.v_simTime.push_back(SimTime());	

		//update variables for (t-1) in next execution
		Kalman.T_prev = Kalman.T_current; 
		Kalman.t_prev = Kalman.t_current;

		Kalman.P_prev = Kalman.P_current; 
		Kalman.m_prev = Kalman.m_current;
		Kalman.noise_prev = Kalman.noise_estimation; 

		Kalman_measured_delay += Kalman.m_current; 

		XR_packet.m_owdg = Kalman.m_current; //we set the packet's Kalman stats
		XR_packet.threshold_gamma = Threshold.gamma; 	

		XR_packet.feedback = true; //to send to the server. 
		XR_packet.last_video_frame_packet = 1; //FIX
				
				
		XR_packet.num_seq = SEQNUMMAX; //last received sequence number will be sent in feedback

			
		
		//printf("******%f -> %f mowdf\n %f ->%f threshold \n", Kalman.m_current, XR_packet.m_owdg, Threshold.gamma, XR_packet.threshold_gamma);
		Threshold.gamma_prev = Threshold.gamma; 
		out(XR_packet);	
	}

};

double XRClient::K_gamma(double mowdg, double gamma_prev){

	double K_d = -0.01;		//    (ku,kd) = (0.01, 0.00018)
	double K_u = 0.00018;		// "guarantees good trade-off between high throughput, delay reduction and inter-protocol fairness from the webrtc congestion paper "

	if(mowdg<gamma_prev){
		return K_d;
	}
	else{
		return K_u; 
	}
};



void XRClient :: sliding_window_routine(trigger_t &){
//1. drop all old packets from vector

	double CurrentTime = SimTime(); 

				//unused at the moment
	//double RTT_slide = 0;
	//double RX_frames_slide = 0; 
	//double MOWDG_slide = 0;
	//double Frameloss_slide = 0; 
	// double no_lost_packets = 0; de


	int no_packets = 0;
	//int no_feedback_packets = 0;  
	for (auto it = sliding_vector_client.begin(); it!=sliding_vector_client.end(); ){
		if(it->Timestamp <= (CurrentTime - TIME_SLIDING_WINDOW_CLIENT))
		{	
			//printf("[DBG_CLIENT] deleting OLD %.1f, SimTime: %.2f\n", it->Timestamp, SimTime());
			it = sliding_vector_client.erase(it);
		}
		else{
			it ++;
			no_packets ++; 
		}

	}

//2. compute metrics over the sliding window
	/*
	std::sort( sliding_vector_client.begin(), sliding_vector_client.end(), compareStructbyNumseq2 );

	printf("***********************Let's see the ordered NUMSEQ: **********************\n");
	int debug_counter = 0; 
	for (auto it = sliding_vector_client.begin(); it!=sliding_vector_client.end(); ){
		if(it->Packet.num_seq != 0 )
		{	
			printf("\t%.1f\n", it->Packet.num_seq);
		}
		it++;
		debug_counter++; 
	}
	printf("\n");

	printf("\tNº of packets inside according to routine: %d\n \tNº of packets inside according to sorted algo %d\n",no_packets, debug_counter); 

	
	sliding_window_timer.Set(SimTime()+0.5);
*/

//3. Make contents of next messages to be the averaged metrics.
// Just use a public struct with variables and make the contents of feedback messages be from there. 




	//TODO: CLIENT METRICS SLIDING WINDOW ON SORTED VECTOR
	//HOWEVER, ORDERED VECTOR WON'T WORK (THE NUMSEQ IS KEPT AT 0 ONCE SORTED FOR SOME REASON!!! )



};

bool compareStructbyNumseq2(const sliding_window_t& a, const sliding_window_t& b) {
    return a.num_seq < b.num_seq; // smallest first
};


#endif
