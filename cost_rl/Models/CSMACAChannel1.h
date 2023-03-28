/*
	CSMACAChannel
*/

#ifndef _CSMACAChannel1_
#define _CSMACAChannel1_

#include "definitions.h"

#define SLOT 9E-6 // microseconds

component CSMACAChannel1 : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		

	public:
		int NumNodes;
		void NextSlot();


	private:
		int sim_transmissions; // simultaneous transmissions
		int current_transmissions;
		double tx_duration, collision_duration;
		
	
	public:
		// Connections
		outport [] void out_slot(SLOT_indicator &slot);	
		inport void in_frame(data_packet &packet);

		// Timers
		Timer <trigger_t> slot_time;
		Timer <trigger_t> rx_time;

		inport inline void new_slot(trigger_t& t1);
		inport inline void reception_time(trigger_t& t2);

		CSMACAChannel1 () { 
			connect slot_time.to_component,new_slot; 
			connect rx_time.to_component,reception_time; }
	
};

void CSMACAChannel1 :: Setup()
{
	printf("CSMACAChannel1 Setup()\n");
};

void CSMACAChannel1 :: Start()
{
	printf("CSMACAChannel1 Start()\n");

	current_transmissions = 0;
	sim_transmissions = 0;
	tx_duration = 0;
	slot_time.Set(SimTime());	
};

void CSMACAChannel1 :: Stop()
{
	printf("CSMACAChannel1 Stop()\n");

};

void CSMACAChannel1 :: new_slot(trigger_t &)
{
	SLOT_indicator slot;

	slot.status=sim_transmissions;

	//printf("%f ------ New Slot -------\n",SimTime());

	sim_transmissions = 0; // Init the counter for the number of simultaneous tx.
	current_transmissions = 0;	
	tx_duration=0;

	for(int n=0;n<NumNodes;n++) out_slot[n](slot);

	rx_time.Set(SimTime());		
}

void CSMACAChannel1 :: reception_time(trigger_t &)
{
	//printf("%f - TX DURATION = %f\n",SimTime(),tx_duration);
	if(sim_transmissions==0) slot_time.Set(SimTime()+SLOT);
	if(sim_transmissions == 1) slot_time.Set(SimTime()+tx_duration);
	if(sim_transmissions > 1) slot_time.Set(SimTime()+collision_duration);
}


void CSMACAChannel1 :: in_frame(data_packet &packet)
{
	//printf("%f - Channel: Packet arrives\n",SimTime()); 
	if(packet.AMPDU_size > current_transmissions) current_transmissions = packet.AMPDU_size;
	//printf("%f - Channel: received frame %d, with SB size = %d, current tx = %d\n",SimTime(),packet.source,packet.AMPDU_size,current_transmissions);
	sim_transmissions++;
	//double current_tx_duration=0;
	/*	
	if(packet.AMPDU_size > 1)
	{
		int m = packet.AMPDU_size;
		current_tx_duration = (192+m*64+80+packet.L)/RATE+m*(272)/RATE+m*SIFS+DIFS+SLOT;
		printf("%d %f\n",m,current_tx_duration-DIFS-SLOT);
	}
	else
	{
		current_tx_duration = (192+80+packet.L)/RATE+SIFS+(272/RATE)+DIFS+SLOT;
		printf("1 %f\n",current_tx_duration-DIFS-SLOT);

	}
	*/
	if(tx_duration < packet.T) 
	{	
		tx_duration = packet.T;
	}
	//collision_duration = (192+M*64+80+packet.L)/RATE+M*(272)/RATE+M*SIFS+DIFS+SLOT;
	collision_duration = packet.T_c;	
	//printf("%f - TX DURATION = %f - %f - %f ---------------- \n",SimTime(),tx_duration,packet.T,collision_duration);

}


#endif
