/*
	Station
*/

#ifndef _STATION_
#define _STATION_

#include "definitions.h"
//#include "FIFO.h"
#include "PHY_model.h"
#include <deque>

component Station : public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		
		int BinaryExponentialBackoff(int attempt);	
		double FrameTransmissionDelay(double TotalBitsToBeTransmitted,int NMPDUs, int station_id);

	public: // Connections
		inport void in_from_app(data_packet &packet);
		outport [] void out_to_wireless(data_packet &packet);	

		inport void in_from_wireless(data_packet &packet);
		outport void out_to_app(data_packet &packet);	

		/*
		inport void in_info_ap(info &info_request);
		outport [] void out_info_ap(info &info_response);

		inport void in_request_ap(info &info_request);
		outport [] void out_response_ap(info &info_response);	
		*/

		// For the DCF
		inport void inline in_slot(SLOT_indicator &slot);
		outport void out_packet(data_packet &frame);

	public:
		int id;		
		double x,y,z;

		int MAX_AMPDU;
		int QL;
		int CWmin;
		int max_BEB_stages;
		double pe;
		double channel_width;
		int SU_spatial_streams;

		double Pt;
		double BitsSymbol[20];
		double CodingRate[20];


		//FIFO test_queue;
		std::deque <data_packet> MAC_queue;
		int qmin; // Minimum number of packets in the buffer to start a tx

		int current_ampdu_size; // Number of packets aggregated in current transmission
		int current_destination;
		int NumberStations;

	private:
		int mode; // 0: idle; 1: in transmission
		int backoff_counter; // Current backoff counter
		int attempts; // Number of attempts per packet
		int device_has_transmitted;
		double T = 0;
		double T_c = 0;

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
		double av_MPDUsize; // Average number of Packets Transmitted per av_MPDUsize?
		double successful; // Number of successful transmissions
		double queue_occupation;	
};

void Station :: Setup()
{
	printf("Station Setup()\n");

};

void Station :: Start()
{
	printf("Station Start()\n");

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
	av_MPDUsize=0;
	queue_occupation=0;


};

void Station :: Stop()
{

//	if(id==1)
//	{
		printf("---------------- Results Station %d----------------\n",id);
		printf("Transmission Probability = %f (%f / %f)|| EB=%f\n",transmission_attempts/slots,transmission_attempts,slots,EB/transmission_attempts);
		printf("Collision Probability = %f (%f / %f)\n",collisions/transmission_attempts,collisions,transmission_attempts);
		printf("Queue Utilization = %f (%f / %f)\n",rho/arrived,rho,arrived);
		printf("Queue Occupation = %f\n",queue_occupation/arrived);
		printf("Service Time = %f | Queueing + Service Delay = %f\n",service_time/successful,queueing_service_delay/successful);
		printf("Load = %f (packets/s) - Througput = %f (packets/s)- Blocking Probability = %f\n",arrived/SimTime(),av_MPDUsize / SimTime(),blocking_prob/arrived);
		printf("Average AMPDU size = %f\n",av_MPDUsize/successful);
//	}

};


void Station :: in_from_app(data_packet &packet)
{
	//printf("%f - New packet (to %d) arrives to Station %d. Packets in the buffer = %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",SimTime(),packet.destination,id,(int)MAC_queue.size());

	arrived++;
	int QueueSize = MAC_queue.size();
	queue_occupation+=QueueSize;
	//printf("%f - New frame. Queue Size = %d \n",SimTime(),QueueSize);
	if(QueueSize >= qmin) 
	{
		rho++;
	}

	if(QueueSize < QL)
	{
		packet.queueing_service_delay = SimTime();
		MAC_queue.push_back(packet);
	}
	else
	{
		blocking_prob++;
	}

};


void Station :: in_slot(SLOT_indicator &slot)
{

	//printf("---- AP IN SLOT? Status = %d----\n",slot.status);

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
			if(traces_on) printf("%f - STA (%d) : Successful Transmission | AMPDU size = %d\n",SimTime(),id,current_ampdu_size);
			mode=0; // Move to not transmitting
			device_has_transmitted = 0; 
			av_MPDUsize+=current_ampdu_size;
			// We remove from the buffer the batch of successful received packets
			data_packet frame_test;
			double queueing_service_delay_aux=0; // to calculate the queueing service delay of each packet
			int packet_queue_index = 0;
			for(int q=0;q<current_ampdu_size;q++)
			{
				//frame_test = MAC_queue.front();
				frame_test = MAC_queue.at(packet_queue_index);
				// To implement here channel errors (not collisions)
				
				if(Random()>pe)
				{	
					MAC_queue.pop_front();			
					queueing_service_delay_aux += (SimTime()-frame_test.queueing_service_delay-SLOT);
					//printf("%f - STA - Packet to AP %d without errors\n",SimTime(),frame_test.destination);
					//for(int n=0;n<NumberStations;n++) out_to_wireless[n](frame_test); // We send each packet to the corresponding destination		
					out_to_wireless[frame_test.destination](frame_test);
				}
				else
				{
					packet_queue_index++;
					//printf("%f - STA - Packet to AP %d with errors\n",SimTime(),frame_test.destination);
				}

			}

			queueing_service_delay_aux = queueing_service_delay_aux / current_ampdu_size;
			queueing_service_delay += queueing_service_delay_aux;
			current_ampdu_size=0; // Quite irrelevant, since we are not transmitting yet
			attempts=0;
			service_time += (SimTime()-aux_service_time-SLOT);
			successful++;

		}

	}
	if(slot.status > 1) // Collision
	{
		//printf("%f - Collision || Mode = %d | BO = %d\n",SimTime(),mode,backoff_counter);
		if(device_has_transmitted == 1)
		{
			if(traces_on) printf("%f - STA (%d) : Collision | Attempts = %d !!!\n",SimTime(),id,attempts);
			device_has_transmitted = 0;
			//attempts++;			
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter; // To calculate the average backoff value
			// mode is kept to 1
			//printf("%f- STA %d re-starts transmission mode with BO=%d \n",SimTime(),id,backoff_counter);
			collisions++; // stat
			
		}
		else
		{
			// Someone else has suffered a collision
		}

	}

	// ######################################################################################################

	if(mode == 0) // Not in transmission mode
	{
		int QueueSize = MAC_queue.size();
		//if((QueueSize >= qmin && continuation==0 && QueueSize >= smin)||(QueueSize >= smin && continuation==1))
		if(QueueSize >= qmin)
		{
			mode = 1; // I satisfy the conditions to switch to transmission mode
			attempts=0; // First attempt.
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter;
			current_ampdu_size = 1; // To update with queue size < QMAX			
			aux_service_time=SimTime();
			//printf("%f- STA %d switchs to transmission mode with BO=%d, Current av_MPDUsize = %d (QS=%d) \n",SimTime(),id,backoff_counter,current_ampdu_size,QueueSize);
		}
	}

	if(mode == 1) // In transmission mode
	{
		//printf("Mode = 1 and backoff counter = %d\n",backoff_counter);		
		if(backoff_counter==0)
		{
			//printf("%f-AP %d is going to start a transmission -------------------------------------------------------------\n",SimTime(),id);
			//printf("%f-Node %d backoff = 0\n",SimTime(),id);
			// Time to sent a frame
			current_ampdu_size = MIN((int)MAC_queue.size(),MAX_AMPDU);

			// 1. Pick the first packet in the buffer. Identify the STA.
			data_packet first_packet_in_buffer = MAC_queue.front();
			current_destination = first_packet_in_buffer.destination; 
			
			// 2. Select up to MAX_AMPDU packets to that STA.
			int BufferSize = MAC_queue.size();			
			int current_ampdu_size_per_station = 0;		
		
			// [0|1|0|1|] -->
			// q = 1; c=0; [0|0|1] --> [1|0|0|1] --> c=1;
			// q = 3; c=1; [1|0|0] [1|1|0|0]; --> c=2  	

			double TotalBitsToBeTransmitted = 0;
			for(int q=0;q<BufferSize;q++)
			{
				data_packet packet_to_check = MAC_queue.at(q); 
				if(current_destination == packet_to_check.destination && current_ampdu_size_per_station < MAX_AMPDU)
				{				
					MAC_queue.erase(MAC_queue.begin()+q);
					MAC_queue.insert(MAC_queue.begin()+current_ampdu_size_per_station,packet_to_check); //.PutPacketIn(packet_to_check,current_ampdu_size_per_station);		
					//printf("************** Removed from %d, and added to %d\n",q,current_ampdu_size_per_station);					
					TotalBitsToBeTransmitted+=packet_to_check.L;
					current_ampdu_size_per_station++;
				}
			}
			int current_ampdu_size_sta = MIN(current_ampdu_size_per_station,MAX_AMPDU);	
			//printf("%f-STA %d | Destination %d | W/O STA = %d | W STA = %d\n",SimTime(),id,current_destination,current_ampdu_size,current_ampdu_size_sta);

			/*
			// Check MCS for the station 
			info request;
			request.station_id = id;
			//printf("%f-AP %d requests info to the STATION %d\n",SimTime(),id,current_destination);
			out_info_ap[current_destination](request); 
			*/

			current_ampdu_size = current_ampdu_size_sta;
			// Calculate duration = f(MCS,ampdu_size, etc.)
			//-------------------------
			// Packets that will be transmitted
			

			FrameTransmissionDelay(TotalBitsToBeTransmitted,current_ampdu_size_sta,current_destination);
			//double T_duration = 10E-3;			
			data_packet frame;
			frame.AMPDU_size = current_ampdu_size_sta;
			frame.source=id;
			frame.T = T;
			frame.T_c = T_c;
			//frame.L = Data_length;
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


void Station :: in_from_wireless(data_packet &packet)
{
	//printf("*********************** %d - %d ********************************************\n",packet.destination,id);
	if(packet.destination == id) out_to_app(packet);	
}


int Station :: BinaryExponentialBackoff(int attempt)
{
	int CW = Random(MIN(pow(2,attempt),pow(2,max_BEB_stages))*(CWmin+1));
	//printf("Attempt = %d | BEB --> CW = %d\n",attempt,CW);
	return CW;	
};

double Station :: FrameTransmissionDelay(double TotalBitsToBeTransmitted, int N_MPDUs, int station_id)
{	

	// station = 0 (AP) in all cases. To improve.
	
	// Effective Pt
	double effPt = Pt;	
	if (SU_spatial_streams > 1) effPt = effPt - 3*SU_spatial_streams;
	if (channel_width > 20) effPt = effPt - 3*(channel_width/20);

	double distance = CalculateDistance(x,y,z,x_AP[station_id],y_AP[station_id],z_AP[station_id]);
	double PL = PathLoss(distance);
	double Pr = effPt - PL;
	RSSI[station_id] = Pr;

	//printf("STA to AP %d: I'm at %f,%f,%f and you are at %f,%f,%f | PL = %f | Pr = %f\n",station_id,x,y,z,x_[station_id],y_[station_id],z_[station_id],PL,Pr);

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
	

	int SU_spatial_streams = 2;
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

	//double 
	T = T_RTS + SIFS + T_CTS + SIFS + T_DATA + SIFS + T_ACK + DIFS + SLOT;
	T_c = T_RTS + SIFS + T_CTS + DIFS + SLOT;
	//printf("0000 Data Rate = %f (%f) and The value of T = %f (T_DATA = %f)| NMPDUS = %d\n",ORate,OBasicRate,T,T_DATA,N_MPDUs);

	return T;
};


#endif
