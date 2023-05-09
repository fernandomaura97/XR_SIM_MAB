/*
	Access Point
*/

#ifndef _ACCESSPOINT_
#define _ACCESSPOINT_

#include "definitions.h"
#include "FIFO.h"
#include "PHY_model.h"


component AccessPoint : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		
		int BinaryExponentialBackoff(int attempt);	
		void FrameTransmissionDelay(double TotalBitsToBeTransmitted,int NMPDUs, int station_id);

	public: // Connections
		inport void in_from_network(data_packet &packet); 
		outport [] void out_to_wireless(data_packet &packet);	

		inport void in_from_wireless(data_packet &packet);
		outport void out_to_network(data_packet &packet);	
	
		// For the DCF
		inport void inline in_slot(SLOT_indicator &slot);
		outport void out_packet(data_packet &frame);

	public:
		int id;		
		double x,y,z;

		int MAX_AMPDU;
		int QL;

		double Pt;
		double BitsSymbol[20];
		double CodingRate[20];


		FIFO MAC_queue;
		int qmin; // Minimum number of packets in the buffer to start a tx
		int CWmin;
		int max_BEB_stages;
		double pe;
		double channel_width;
		int SU_spatial_streams;

		int current_ampdu_size; // Number of packets aggregated in current transmission
		int current_destination;
		int NumberStations;

	private:
		int mode; // 0: idle; 1: in transmission
		int backoff_counter; // Current backoff counter
		int attempts; // Number of attempts per packet
		int device_has_transmitted;
		double T = 0; // Duration of a transmission
		double T_c = 0; // Duration of a collision

	public: //statistics - results
		double transmission_attempts;
		double collisions;
		double arrived; // number of arrived packets
		double rho; // to compute the prob that the system is 'busy'. Not default meaning.
		double service_time;
		double queueing_service_delay;
		double aux_service_time;
		double slots;
		double EB; // average backoff value
		double blocking_prob;
		double avAMPDU_size; // Average number of Packets Transmitted per avAMPDU_size?
		double successful; // Number of successful transmissions
		double queue_occupation;
	
};

void AccessPoint :: Setup()
{
	printf("Access Point Setup()\n");

};

void AccessPoint :: Start()
{
	printf("Access Point Start()\n");

	mode = 0;
	device_has_transmitted=0;
	backoff_counter=(int)Random(CWmin);
	attempts = 0;	
	//printf("%f - Node %d ready\n",SimTime(),id);

	current_ampdu_size=0; 
	collisions = 0;
	transmission_attempts = 0;
	arrived = 0;
	rho = 0;
	service_time = 0;
	aux_service_time = 0;
	slots=0;
	EB=0;
	blocking_prob=0;

	successful=0;
	avAMPDU_size=0;
	queue_occupation=0;



};

void AccessPoint :: Stop()
{


	if(id==0)
	{
		printf("---------------- Results AP----------------\n");
		printf("Transmission Probability = %f (%f / %f)|| EB=%f\n",transmission_attempts/slots,transmission_attempts,slots,EB/transmission_attempts);
		printf("Collision Probability = %f (%f / %f)\n",collisions/transmission_attempts,collisions,transmission_attempts);
		printf("Queue Utilization = %f (%f / %f)\n",rho/arrived,rho,arrived);
		printf("Queue Occupation = %f\n",queue_occupation/arrived);
		printf("Service Time = %f | Queueing + Service Delay = %f\n",service_time/successful,queueing_service_delay/successful);
		printf("Load = %f (packets/s) - Througput = %f (packets/s)- Blocking Probability = %f\n",arrived/SimTime(),avAMPDU_size / SimTime(),blocking_prob/arrived);
		printf("Average AMPDU size = %f\n",avAMPDU_size/successful);

	}

};


void AccessPoint :: in_from_network(data_packet &packet)
{

	if(traces_on) printf("%f - AP : New packet arrives from the network directed to STA %d | AP Tx Buffer = %d | Packet in Frame = %d\n",SimTime(),packet.destination,MAC_queue.QueueSize(),packet.num_packet_in_the_frame);

	arrived++;
	int QueueSize = MAC_queue.QueueSize();
	queue_occupation+=QueueSize;

	if(QueueSize >= qmin) 
	{
		rho++;
	}

	if(QueueSize < QL)
	{
		packet.queueing_service_delay = SimTime();
		MAC_queue.PutPacket(packet);
	}
	else
	{
		blocking_prob++;
	}

};


void AccessPoint :: in_slot(SLOT_indicator &slot)
{

	slots++;

	// ######################################################################################################
	// Implications of the last slot: idle (0), successful (1) or collision (>1)
	
	if(slot.status == 0) // Idle
	{
		//printf("%f - Channel Empty - Mode = %d - BO = %d\n",SimTime(),mode,backoff_counter);
	}
	if(slot.status == 1) // Successful transmission
	{
		//printf("%f - Successful Transmission || Mode = %d | BO = %d\n",SimTime(),mode,backoff_counter);
		if(device_has_transmitted == 1)
		{
			//printf("%f - AP : Successful Transmission (%d) | AMPDU =  %d \n",SimTime(),id,current_ampdu_size);
			mode=0; // Move to not transmitting
			device_has_transmitted = 0; 
			avAMPDU_size+=current_ampdu_size; // Statistics, to improve
			// We remove from the buffer the batch of successful received packets
			data_packet frame_test;
			double queueing_service_delay_aux=0; // to calculate the queueing service delay of each packet
			int packet_queue_index = 0;
			for(int q=0;q<current_ampdu_size;q++)
			{
				//frame_test = MAC_queue.GetFirstPacket();
				frame_test = MAC_queue.GetPacketAt(packet_queue_index);				
				if(Random()>pe)
				{				
					// To implement here channel errors (not collisions)
					//MAC_queue.DelFirstPacket();			
					MAC_queue.DeletePacketIn(packet_queue_index);
					queueing_service_delay_aux += (SimTime()-frame_test.queueing_service_delay-SLOT);
					//for(int n=0;n<NumberStations;n++) 
					//{	
						//printf("%f - AP tranmits packet to STA %d (Video packet = %d)\n",SimTime(),frame_test.destination,frame_test.num_packet_in_the_frame);
						out_to_wireless[frame_test.destination](frame_test); // We send each packet to all stations (no broadcast)		
					//}
				}
				else
				{
					packet_queue_index++;
					//printf("%f - AP - Packet to STA %d with errors (Video packet = %d)\n",SimTime(),frame_test.destination,frame_test.num_packet_in_the_frame);
				}

			}

			queueing_service_delay_aux = queueing_service_delay_aux / current_ampdu_size;
			queueing_service_delay += queueing_service_delay_aux;
			current_ampdu_size=0; // Quite irrelevant, since we are not transmitting yet
			attempts=0;
			service_time += (SimTime()-aux_service_time-SLOT);
			successful++;
			mode = 0;

		}

	}
	if(slot.status > 1) // Collision
	{
		//printf("%f - Collision || Mode = %d | BO = %d\n",SimTime(),mode,backoff_counter);
		if(device_has_transmitted == 1)
		{
			//printf("%f - AP : Collision (%d) || Attempts = %d!!!\n",SimTime(),id,attempts);
			device_has_transmitted = 0;
			//attempts++;			
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter; // To calculate the average backoff value
			// mode is kept to 1
			//printf("%f- AP %d re-starts transmission mode with BO=%d \n",SimTime(),id,backoff_counter);
			collisions++; // stat
			
		}
		else
		{
			// Collision from others
		}

	}

	// ######################################################################################################

	if(mode == 0) // Not in transmission mode
	{
		int QueueSize = MAC_queue.QueueSize();
		if(QueueSize >= qmin)
		{
			mode = 1; // I satisfy the conditions to switch to transmission mode
			attempts=0; // First attempt.
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter;
			current_ampdu_size = 1; // To update with queue size < QMAX			
			aux_service_time=SimTime(); // We move the packet to the transmitter, service time starts
			//printf("%f - AP %d switchs to transmission mode with BO=%d, Current avAMPDU_size = %d (QS=%d) \n",SimTime(),id,backoff_counter,current_ampdu_size,QueueSize);
		}
	}

	if(mode == 1) // In transmission mode
	{
		//printf("%f - AP : Mode = 1 and backoff counter = %d\n",SimTime(),backoff_counter);		
		if(backoff_counter==0)
		{
			// Time to sent a frame
			current_ampdu_size = MIN(MAC_queue.QueueSize(),MAX_AMPDU);

			int BufferSize = MAC_queue.QueueSize();			

			// 1. Pick the first packet in the buffer. Identify the STA.
			data_packet first_packet_in_buffer = MAC_queue.GetFirstPacket();
			// Random Packet in the buffer
			//data_packet first_packet_in_buffer = MAC_queue.GetPacketAt(Random(BufferSize)); 

			current_destination = first_packet_in_buffer.destination; 
			
			// 2. Select up to MAX_AMPDU packets to that STA.

			int current_ampdu_size_per_station = 0;		
		
			// [0|1|0|1|] -->
			// q = 1; c=0; [0|0|1] --> [1|0|0|1] --> c=1;
			// q = 3; c=1; [1|0|0] [1|1|0|0]; --> c=2  	

			double TotalBitsToBeTransmitted = 0;
			for(int q=0;q<BufferSize;q++)
			{
				data_packet packet_to_check = MAC_queue.GetPacketAt(q); 
				if(current_destination == packet_to_check.destination && current_ampdu_size_per_station < MAX_AMPDU)
				{				
					MAC_queue.DeletePacketIn(q);
					MAC_queue.PutPacketIn(packet_to_check,current_ampdu_size_per_station);		
					//printf("************** Removed from %d, and added to %d\n",q,current_ampdu_size_per_station);					
					TotalBitsToBeTransmitted+=packet_to_check.L;
					current_ampdu_size_per_station++;
				}
			}
			int current_ampdu_size_sta = MIN(current_ampdu_size_per_station,MAX_AMPDU);	


			/*
			// Check MCS for the station 
			info request;
			//printf("%f-AP %d requests info to the STATION %d\n",SimTime(),id,current_destination);
			out_info_ap[current_destination](request); 
			*/
			current_ampdu_size = current_ampdu_size_sta;
			
			//double T_duration = FrameTransmissionDelay(TotalBitsToBeTransmitted,current_ampdu_size_sta,current_destination);
			FrameTransmissionDelay(TotalBitsToBeTransmitted,current_ampdu_size_sta,current_destination);
			data_packet frame;
			frame.AMPDU_size = current_ampdu_size_sta;
			frame.source=id;
			frame.T = T;
			frame.T_c = T_c;

			//printf("%f - AP %d Transmits | Destination %d | AMPDU =  %d | Duration = %f | TotalBits = %f\n",SimTime(),id,current_destination,current_ampdu_size_sta,T,TotalBitsToBeTransmitted);

			out_packet(frame); // To the channel!!!
			attempts++; 
			device_has_transmitted=1;
			transmission_attempts++; // stat
		}
		else
		{
			backoff_counter--;

		}
	}

};


void AccessPoint :: in_from_wireless(data_packet &packet)
{
	// Here we should have an interface....
	out_to_network(packet);	
}

int AccessPoint :: BinaryExponentialBackoff(int attempt)
{
	int CW = Random(MIN(pow(2,attempt),pow(2,max_BEB_stages))*(CWmin+1));
	return CW;	
};

void AccessPoint :: FrameTransmissionDelay(double TotalBitsToBeTransmitted, int N_MPDUs, int station_id)
{	

	// Effective Pt
	double effPt = Pt;	
	if (SU_spatial_streams > 1) effPt = effPt - 3*SU_spatial_streams;
	if (channel_width > 20) effPt = effPt - 3*(channel_width/20);

	double distance = CalculateDistance(x,y,z,x_[station_id],y_[station_id],z_[station_id]);
	double PL = PathLoss(distance);
	double Pr = effPt - PL;

	//printf("AP to STA %d: I'm at %f,%f,%f and you are at %f,%f,%f | Distance = %f | PL = %f\n",station_id,x,y,z,x_[station_id],y_[station_id],z_[station_id],distance,PL);

	if (Pr < -82)
	{
		//printf("************************* There is no conectivity ************************* [We assume MCS 1 and pe = 1]\n");
		BitsSymbol[station_id] = 1;
		CodingRate[station_id] = (double) 1/2;
		pe = 0.9;
	}

	if( Pr >= -82 && Pr < -79)
	{
		BitsSymbol[station_id] = 1;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -79 && Pr < -77)
	{
		BitsSymbol[station_id] = 2;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -77 && Pr < -74)
	{
		BitsSymbol[station_id] = 2;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -74 && Pr < -70)
	{
		BitsSymbol[station_id] = 4;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -70 && Pr < -66)
	{
		BitsSymbol[station_id] = 4;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -66 && Pr < -65)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -65 && Pr < -64)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 2/3;
	}	 
	if( Pr >= -64 && Pr < -59)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -59 && Pr < -57)
	{
		BitsSymbol[station_id] = 8;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -57 && Pr < -55)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 5/6;
	}	 
	if( Pr >= -55 && Pr < -53)
	{
		BitsSymbol[station_id] = 10;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -53)
	{
		BitsSymbol[station_id] = 10;
		CodingRate[station_id] = (double) 5/6;
	}	 
	

	int Subcarriers = 980; // https://www.arubanetworks.com/assets/wp/WP_802.11AX.pdf, page 12
	if(channel_width == 80) Subcarriers = 980;
	if(channel_width == 40) Subcarriers = 468;
	if(channel_width == 20) Subcarriers = 234;

	//int SU_spatial_streams = 2;
	double ORate = SU_spatial_streams * BitsSymbol[station_id] * CodingRate[station_id] * Subcarriers;
	double OBasicRate = (double) 1/2 * 1 * 48;

	double L = TotalBitsToBeTransmitted/N_MPDUs;
	
	int SF = 16;
	int TB = 18;
	int MD = 32;
	int MAC_H_size = 240;

	double T_RTS = Legacy_PHY_duration + ceil((SF+160+TB)/OBasicRate)*4E-6;
	double T_CTS = Legacy_PHY_duration + ceil((SF+112+TB)/OBasicRate)*4E-6;
	double T_DATA = PHY_duration + ceil((SF+N_MPDUs*(MD + MAC_H_size + L) + TB)/ORate)*16E-6;
	double T_ACK = Legacy_PHY_duration + ceil((SF+240+TB)/OBasicRate)*4E-6;

	//double T = T_RTS + SIFS + T_CTS + SIFS + T_DATA + SIFS + T_ACK + DIFS + SLOT;
	T = T_RTS + SIFS + T_CTS + SIFS + T_DATA + SIFS + T_ACK + DIFS + SLOT;
	T_c = T_RTS + SIFS + T_CTS + DIFS + SLOT;	
	//printf("%f - AP : Data Rate = %f | Basic Rate = %f | T = %f (T_DATA = %f) T_c = %f | NMPDUS = %d\n",SimTime(),ORate,OBasicRate,T,T_DATA,T_c,N_MPDUs);

	//return T;
};


#endif
