/*
	Network
*/

// Need to think how can we add here propagation time (or both, just called network time)

#ifndef _NETWORK_
#define _NETWORK_

#include "definitions.h"

component Network : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		

	public: // Connections
		inport void in_from_apps(data_packet &packet);
		outport [] void out_to_apps(data_packet &packet);
	
		inport void in_from_APs(data_packet &packet);
		outport [] void out_to_APs(data_packet &packet);	


		// Timer
		Timer <trigger_t> transmission_time_DL;
		inport inline void end_packet_transmission_DL(trigger_t& t); // action that takes place when timer expires
		Timer <trigger_t> transmission_time_UL;
		inport inline void end_packet_transmission_UL(trigger_t& t); // action that takes place when timer expires

		Network () { 
			connect transmission_time_UL.to_component,end_packet_transmission_UL;
			connect transmission_time_DL.to_component,end_packet_transmission_DL; }				 

	public:
		//FIFO TxBuffer;
		std::deque <data_packet> TxBuffer_DL;
		std::deque <data_packet> TxBuffer_UL;
		long unsigned int MaxPackets = 10000;
		double Rate;

	public:
		double LinkRate; 
		//int packets_in_transit = 0;
	
};

void Network :: Setup()
{
	printf("Network Setup()\n");

};

void Network :: Start()
{
	printf("Network Start()\n");
};

void Network :: Stop()
{
	printf("Network Stop()\n");
};

void Network :: in_from_apps(data_packet &packet)
{
	
	//printf("%f - Net : New DL packet from %d arrives to the Network. Packets in the buffer = %lu | Packet in frame = %d\n",SimTime(),packet.source,TxBuffer_UL.size(),packet.num_packet_in_the_frame);
 
	////if(TxBuffer.QueueSize() < MaxPackets)
	if(TxBuffer_DL.size() < MaxPackets)	
	{
		TxBuffer_DL.push_back(packet);
	}
	else
	{
		//printf("Packet Dropped\n");
	}
	
	if(TxBuffer_DL.size()==1)
	{
		transmission_time_DL.Set(SimTime()+(packet.L/Rate));
	}
	
};

void Network :: end_packet_transmission_DL(trigger_t &)
{
	//printf("%f - Network: Packet Transmitted DL \n",SimTime());
	data_packet tx_packet = TxBuffer_DL.front();
	out_to_APs[0](tx_packet); // Single AP, to update
	TxBuffer_DL.pop_front();
	if(TxBuffer_DL.size()>0)
	{
		// Start Transmission
		transmission_time_DL.Set(SimTime()+(tx_packet.L/Rate));
	}

};

void Network :: in_from_APs(data_packet &packet)
{
	
	//printf("%f - Net : New UL packet from %d arrives to the Network. Packets in the buffer = %lu\n",SimTime(),packet.source,TxBuffer_UL.size());

	//if(TxBuffer.QueueSize() < MaxPackets)

	if(TxBuffer_UL.size() < MaxPackets)	
	{
		TxBuffer_UL.push_back(packet);
	}
	else
	{
		//printf("Packet Dropped UL\n");
	}
	
	if(TxBuffer_UL.size()==1)
	{
		transmission_time_UL.Set(SimTime()+(packet.L/Rate));
	}
	
};

void Network :: end_packet_transmission_UL(trigger_t &)
{
	//printf("%f - Network: Packet Transmitted UL \n",SimTime());
		
	data_packet tx_packet = TxBuffer_UL.front();
	out_to_apps[tx_packet.destination_app](tx_packet); 
	TxBuffer_UL.pop_front();
	if(TxBuffer_UL.size()>0)
	{
		// Start Transmission
		transmission_time_UL.Set(SimTime()+(tx_packet.L/Rate));
	}


};


#endif
