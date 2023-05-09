/*
	Traffic Generator APP. 
*/

#ifndef _TRAFFICGENERATOR_
#define _TRAFFICGENERATOR_
		
#include "definitions.h"

component TrafficGeneratorApp : public TypeII
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

		TrafficGeneratorApp () { connect inter_packet_timer.to_component,new_packet; }


	public: // Input parameters
		int L_data;
		int id; // sender
		int destination;
		double Load; // bps
		int mode; //0 Poisson, 1 CBR
		int node_attached;
		int source_app;
		int destination_app;


	private:
		double tau; //

	public:
		double generated_packets = 0;
		double received_packets = 0;
		double avDelay = 0;
		double avLreceived = 0;

};

void TrafficGeneratorApp :: Setup()
{
	printf("Traffic Generation APP Setup()\n");
};

void TrafficGeneratorApp :: Start()
{
	printf("Traffic Generation APP Source Start()\n");

	tau = (double) L_data/Load;
	printf("%f\n",tau);
	inter_packet_timer.Set(SimTime()+Exponential(tau));

};
	
void TrafficGeneratorApp :: Stop()
{
	printf("------------------------ TGAPP %d Results ------------------------\n",id);
	printf("GTAPP %d: Number of Generated Packets = %f | Number of Received Packets = %f\n",id,generated_packets,received_packets);
	printf("GTAPP %d: Load = %f \n",id,generated_packets*L_data/SimTime());
	printf("GTAPP %d: Received Traffic = %f \n",id,avLreceived/SimTime());
	printf("Av. Packet Delay = %f\n",avDelay/received_packets);

};


void TrafficGeneratorApp :: new_packet(trigger_t &)
{
	if(traces_on==1) printf("%.9f - Traffic Generation APP %d - New Generated Packet to node %d and app %d\n",SimTime(),id,destination,destination_app);
	data_packet new_gen_packet;
	new_gen_packet.L_data = L_data;
	//new_gen_packet.L_data = 100;
	new_gen_packet.L = 100 + L_data;
	new_gen_packet.source = node_attached;
	new_gen_packet.destination = destination;
	new_gen_packet.source_app = source_app;
	new_gen_packet.destination_app = destination_app;	
	new_gen_packet.sent_time = SimTime();
	
	generated_packets++;
	out(new_gen_packet);

	if(mode==0) inter_packet_timer.Set(SimTime()+Exponential(tau));	
	else inter_packet_timer.Set(SimTime()+tau);
};

void TrafficGeneratorApp :: in(data_packet &packet)
{
	if(traces_on) printf("%f - Traffic Generation APP %d - Packet Received from %d \n",SimTime(),id,packet.source);
	received_packets++;
	avDelay += SimTime() - packet.sent_time;
	//avLreceived += packet.L;
	avLreceived += packet.L_data;

}


#endif
