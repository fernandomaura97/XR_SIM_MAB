/*
	Poisson source. 
*/

#ifndef _XRClient_
#define _XRClient_
		
#include "definitions.h"
#include <algorithm>
#include <numeric>



component XRClient : public TypeII
{
	
	public: // Default functions
		void Setup();
		void Start();
		void Stop();

	public: // Connections
		outport void out(data_packet &packet);
		inport void in(data_packet &packet);	

		// Timer
		Timer <trigger_t> inter_packet_timer;

		inport inline void new_packet(trigger_t& t); // action that takes place when timer expires

		XRClient () { connect inter_packet_timer.to_component,new_packet; }


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
		double packets_rx_video_frames[10000]={0};
		double VideoFramesReceived=0;
		double VideoFramesFullReceived=0;

		double mean_VFD = 0;
		double p99th_VFD = 0;

		

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
	avDelay += SimTime()-packet.sent_time;
	packet_times.push_back(SimTime()-packet.sent_time);
	avRxPacketSize +=packet.L_data;

	// Probability of not receiving a video frame completely
	//printf("Packet in the frame = %d\n",packet.num_packet_in_the_frame);


	if(packet.rtt==true){
			//set packet function to trigger in 50E-3 seconds, however queue may be implemented with stable rate (fps)
			
			countt +=1; 
			sink_timer.Set(SimTime() + 50E-3); // TODO: This time of 50E-3 is arbitrary for the time it takes for the packet to exit the sink

			rtt_packet.sink_time = SimTime() + 50E-3; 
			
			//here would be a good place for Wifi "backoff", delays etc
			memcpy(&rtt_packet.L, &packet.L, sizeof(packet.L)); //copy all info inside packet into buffer struct
			memcpy(&rtt_packet.q_elapsed, &packet.q_elapsed, sizeof(packet.q_elapsed));

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
			packet.Kalman_p = Kalman; //we set the packet's Kalman stats
			//abs_m = std::fabs(Kalman.m_current); this was for threshold  unneeded
		}
	else{packet.feedback = false; }

			//KALMAN FILTER END

	if(packet.video_frame_seq < 10000)
	{
		if(packets_rx_video_frames[(int) packet.video_frame_seq] == 0) VideoFramesReceived++;
		
		packets_rx_video_frames[(int) packet.video_frame_seq]++;
		if(packets_rx_video_frames[(int) packet.video_frame_seq] == packet.NumPacketsPerFrame)
		{
			VideoFramesFullReceived++;
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
	*/

	

	// For interactive (correlated traffic: Last packet, or random
	if(packet.last_video_frame_packet==1)
	//if(Random()<0.1)	
	{
		if(traces_on) printf("%f - XR client %d . UL packet for RTT\n",SimTime(),id);
		data_packet XR_packet = packet;
		XR_packet.L = 20*8 + L_data;
		XR_packet.source = node_attached;
		XR_packet.destination = destination;
		XR_packet.source_app = source_app;
		XR_packet.destination_app = destination_app;
		XR_packet.sent_time = SimTime();
		// To compute RTT;
		XR_packet.TimeSentAtTheServer = packet.sent_time;
		XR_packet.TimeReceivedAtTheClient = SimTime();
		XR_packet.frames_received = VideoFramesFullReceived;
		
		out(XR_packet);	
	}

	
};


#endif
