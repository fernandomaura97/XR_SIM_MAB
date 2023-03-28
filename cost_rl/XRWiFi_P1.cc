#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "./COST/cost.h"

#include <deque>

#include "./Models/definitions.h"
#include "./Models/Network.h"
#include "./Models/TrafficGeneratorApp.h"
#include "./Models/AccessPoint.h"
#include "./Models/Station.h"
#include "./Models/CSMACAChannel1.h"
#include "./Models/XRServer.h"
#include "./Models/XRClient.h"

//int test_global[100];

double x_AP[10];
double y_AP[10];
double z_AP[10];

double x_[100];
double y_[100];
double z_[100];

int traces_on = 0;

double RSSI[100]; // Received Power at the destination


// added just to test a preliminary adaptive control
double test_average_delay_decision[10] = {0};
double test_frames_received[10] = {0};


component XRWiFisim : public CostSimEng
{
	public:
		void Setup(int NXR, int fps, double LoadXR, int LXR, int NBG, double BGLoad, int LBG, int BG_mode,int x, int RCA);
		void Start();		
		void Stop();
	
	public:
		AccessPoint [] AP;
		Station [] STA;
		CSMACAChannel1 channel1;
		XRServer [] XRs;
		XRClient [] XRc;
		TrafficGeneratorApp [] TGApp;
		Network Net;

		// Input parameters
		int distance_ = 0;
		int NXR_ = 0;
		int fps_ = 0;
		double LoadXR_ = 0;
		int NBG_ = 0;
		double BGLoad_ = 0;
		int RCA_ = 0;
		int BG_mode_ = -1;
};

void XRWiFisim :: Setup(int NXR, int fps, double LoadXR, int LXR, int NBG, double BGLoad, int LBG, int BG_mode,int x, int RCA)
{

	distance_ = x;
	NXR_ = NXR;
	fps_ = fps;
	LoadXR_ = LoadXR;
	BGLoad_ = BGLoad;
	NBG_ = NBG;
	RCA_ = RCA;
	BG_mode_ = BG_mode;

	printf("---- XR Wi-Fi sim : Setup ----\n");
 

	// XR traffic sources
	XRs.SetSize(NXR);
	for(int n=0;n<NXR;n++)
	{
		XRs[n].id = n;
		XRs[n].node_attached = 0;
		XRs[n].Load = LoadXR;
		XRs[n].L_data = 1450*8;
		XRs[n].destination = n; // STA 1
		XRs[n].fps = fps;
		XRs[n].source_app = n;
		XRs[n].destination_app = n;
		XRs[n].rate_control_activated = RCA;
	}

	XRc.SetSize(NXR);
	for(int n=0;n<NXR;n++)
	{
		XRc[n].id = n; 
		XRc[n].node_attached = n;	
		XRc[n].Load = 0.5E6; // not used
		XRc[n].L_data = 220*8;
		XRc[n].destination = 0; // the AP
		XRc[n].source_app = n;
		XRc[n].destination_app = n;
		XRc[n].fps = fps;
	}

	// Background Traffic
	int aux_BGDL = 1;
	int aux_BGUL = 0;	
	if(BG_mode == 1)
	{
		aux_BGDL=0;
		aux_BGUL=1;
	} 
	if(BG_mode == 2)
	{
		aux_BGDL=1;
		aux_BGUL=1;
	} 
	

	TGApp.SetSize(2*NBG);
	for(int n=0;n<NBG;n++)
	{
		TGApp[n].Load=aux_BGDL*BGLoad;
		TGApp[n].L_data=LBG;
		TGApp[n].id = NXR+n; 
		TGApp[n].node_attached = 0;
		TGApp[n].destination = NXR+n; // Station 2
		TGApp[n].mode = 0;
		TGApp[n].source_app = NXR+n;
		TGApp[n].destination_app = 0;
	}

	for(int n=0;n<NBG;n++)
	{
		TGApp[NBG+n].Load=aux_BGUL*BGLoad;
		TGApp[NBG+n].L_data=LBG;
		TGApp[NBG+n].id = NXR+n; // Attached to STA 2
		TGApp[NBG+n].node_attached = NXR+n;
		TGApp[NBG+n].destination = 0; // the AP
		TGApp[NBG+n].mode = 0;
		TGApp[NBG+n].source_app = 0;
		TGApp[NBG+n].destination_app = NXR+n;
	}

	int index = 0;
	AP.SetSize(1);
	for(int n=0;n<1;n++)
	{
		AP[n].id = n;
		AP[n].x=0;
		AP[n].y=0;
		AP[n].z=2;
		AP[n].NumberStations=1+NXR;
		AP[n].Pt = 20; //dBm
		AP[n].qmin = 1;
		AP[n].QL = 1000;
		AP[n].MAX_AMPDU = 128;
		AP[n].CWmin = 15;
		AP[n].max_BEB_stages = 6;
		AP[n].pe=0.1;  // Working (but frame receiving model fails)
		AP[n].channel_width = 80; // MHz
		AP[n].SU_spatial_streams = 2;

		AP[n].out_to_wireless.SetSize(NXR+NBG); // Single STA in this example
		x_AP[index]=AP[n].x;
		y_AP[index]=AP[n].y;
		z_AP[index]=AP[n].z;
		index++;
	}

	int NumberStations=NXR+NBG;
	STA.SetSize(NumberStations);

	// XR source
	for(int n=0;n<NXR;n++)
	{
		STA[n].id = n;
		STA[n].x=x;
		STA[n].y=0;
		STA[n].z=2;
		STA[n].NumberStations=1; // To improve
		STA[n].Pt = 20; //dBm
		STA[n].qmin = 1;
		STA[n].QL = 150;
		STA[n].MAX_AMPDU = 64;
		STA[n].CWmin = 15;
		STA[n].max_BEB_stages = 6;
		STA[n].pe=0.1; 
		STA[n].channel_width = 80; // MHz
		STA[n].SU_spatial_streams = 2;

		STA[n].out_to_wireless.SetSize(1); 
		x_[n]=STA[n].x;
		y_[n]=STA[n].y;
		z_[n]=STA[n].z;
		index++;
	}

	// Background STAs
	for(int n=NXR;n<NumberStations;n++)
	{
		STA[n].id = n;
		STA[n].x=0;
		STA[n].y=5;
		STA[n].z=2;
		STA[n].NumberStations=1; // To improve
		STA[n].Pt = 20; //dBm
		STA[n].qmin = 1;
		STA[n].QL = 150;
		STA[n].MAX_AMPDU = 64;
		STA[n].CWmin = 15;
		STA[n].max_BEB_stages = 6;
		STA[n].pe=0.1; 
		STA[n].channel_width = 80; // MHz
		STA[n].SU_spatial_streams = 2;

		STA[n].out_to_wireless.SetSize(1); 
		x_[n]=STA[n].x;
		y_[n]=STA[n].y;
		z_[n]=STA[n].z;
		index++;
	}

	Net.Rate = 1000E6;
	Net.out_to_apps.SetSize(NXR+NBG);
	Net.out_to_APs.SetSize(1);

	channel1.NumNodes = 1+NXR+NBG;
	channel1.out_slot.SetSize(1+NXR+NBG);


	// APPs to Network

	for(int n=0;n<NXR;n++)
	{
		connect XRs[n].out,Net.in_from_apps;
		connect Net.out_to_apps[n],XRs[n].in;
	}


	for(int n=0;n<NBG;n++)
	{
		connect TGApp[n].out,Net.in_from_apps;
		connect Net.out_to_apps[NXR+n],TGApp[n].in;
	}


	// Net to APP

	connect Net.out_to_APs[0],AP[0].in_from_network;	
	connect AP[0].out_to_network,Net.in_from_APs;

	// Connections between AP and STAs

	for(int n=0;n<NumberStations;n++)
	{
		connect AP[0].out_to_wireless[n],STA[n].in_from_wireless;
		connect STA[n].out_to_wireless[0],AP[0].in_from_wireless;	
	}


	// Connections STAs to APPs

	for(int n=0;n<NXR;n++)	
	{
		connect STA[n].out_to_app,XRc[n].in;
		connect XRc[n].out,STA[n].in_from_app;
	}

	for(int n=0;n<NBG;n++)	
	{		
		connect STA[NXR+n].out_to_app,TGApp[NBG+n].in;
		connect TGApp[NBG+n].out,STA[NXR+n].in_from_app;
	}

	// Connection to Channel block

	connect AP[0].out_packet,channel1.in_frame;
	connect channel1.out_slot[0],AP[0].in_slot;

	for(int n=0;n<NumberStations;n++)
	{	
		connect STA[n].out_packet,channel1.in_frame;
		connect channel1.out_slot[n+1],STA[n].in_slot;
	}

	printf("----- Wi-FiSim Setup completed ----- Los!\n");

};

void XRWiFisim:: Start()
{
	printf("Start\n");

};

void XRWiFisim:: Stop()
{
	printf("########################################################################\n");
	printf("------------------------ XRWi-Fisim Results ----------------------------\n");
	printf("AP: RSSI = %f | Packet AP Delay = %f\n",RSSI[0],AP[0].queueing_service_delay/AP[0].successful);
	printf("RTT = %f | Blocking Prob AP = %f\n",XRs[0].avRTT/XRs[0].received_packets,AP[0].blocking_prob/AP[0].arrived);
	printf("########################################################################\n");

	// Video Frame Delay (mean, 99th)
	// RTT
	// Successful RX rate
	// Throughput

	// A-MPDU size
	// Transmission prob
	// Collision prob
	// RSSI

	printf("-------------- Results only for stream '0' --------------\n");
	printf("Input parameters: NXR = %d | distance = %d | LoadXR = %f | NBG = %d | LoadBG = %f | BGmode = %d\n",NXR_,distance_,LoadXR_,NBG_,BGLoad_,BG_mode_);
	printf("Video Frame Delay: Average = %f | 99th = %f | S = %f | Thoughput = %f\n",XRc[0].mean_VFD,XRc[0].p99th_VFD,XRc[0].VideoFramesFullReceived/XRc[0].VideoFramesReceived,XRc[0].avRxPacketSize/SimTime());
	printf("Av A-MPDU size = %f | Tx prob = %f | Coll prob = %f | Buffer size = %f | RSSI = %f \n",AP[0].avAMPDU_size/AP[0].successful,AP[0].transmission_attempts/AP[0].slots,AP[0].collisions/AP[0].transmission_attempts,AP[0].queue_occupation/AP[0].arrived,RSSI[0]);

	int s_NXR = NXR_;
	int s_dist = distance_;
	double s_LoadXR = LoadXR_;
	int s_NBG = NBG_;
	double s_LoadBG = BGLoad_;
	double s_avVFDelay = XRc[0].mean_VFD;
	double s_99VFDelay = XRc[0].p99th_VFD;
	double s_Fraction = XRc[0].VideoFramesFullReceived/XRc[0].VideoFramesReceived;
	double s_Throughput = XRc[0].avRxPacketSize/SimTime();
	double s_avMPDU = AP[0].avAMPDU_size/AP[0].successful;
	double s_txprob = AP[0].transmission_attempts/AP[0].slots;
	double s_collprob = AP[0].collisions/AP[0].transmission_attempts;
	double s_BufferSize = AP[0].queue_occupation/AP[0].arrived;
	double s_RSSI = RSSI[0];
	int s_Control = RCA_;

	FILE *XRWiFisim1_results;
	XRWiFisim1_results = fopen("Results/PaperXRWiFiSim1.txt","at");
	fprintf(XRWiFisim1_results,"%d %d %f %d %d %f %f %f %f %f %f %f %f %f %f %d\n",s_NXR,s_dist,s_LoadXR,s_NBG,BG_mode_,s_LoadBG,s_avVFDelay,s_99VFDelay,s_Fraction,s_Throughput,s_avMPDU,s_txprob,s_collprob,s_BufferSize,s_RSSI,s_Control);
	fclose(XRWiFisim1_results);
 
	printf("RSSIs: %f %f %f\n",RSSI[0],RSSI[1],RSSI[2]);

	/*
	FILE *XRWiFisim1_results;
	XRWiFisim1_results = fopen("Results/XRWiFisim1_results.txt","at");
	fprintf(XRWiFisim1_results,"%f %f %f %f\n",RSSI[0],AP[0].queueing_service_delay/AP[0].successful,XRs[0].avRTT/XRs[0].received_packets,AP[0].blocking_prob/AP[0].arrived);
	fclose(XRWiFisim1_results);
	*/
};

// ---------------------------------------

int main(int argc, char *argv[])
{
	// seed, SimTime, NXR, fps, Load, L

	int seed = atoi(argv[1]);
	double STime = atof(argv[2]);

	int NXR = atoi(argv[3]);
	int fps = atoi(argv[4]);
	double XRLoad = atof(argv[5]);
	int distanceXR = atoi(argv[6]);

	int NBG = atoi(argv[7]);
	double BGLoad = atof(argv[8]);
	int LBG = atoi(argv[9]);
	int BG_mode = atoi(argv[10]);

	int RCA = atoi(argv[11]);

	printf("---- XRWiFisim1 ----\n");
	printf("Seed = %d | SimTime = %f | Rate Control Activated = %d\n",seed,STime,RCA);
	printf("Input Parameters: XR sources: NXR = %d | fps = %d | XRLoad = %f | Distance = %d\n",NXR,fps,XRLoad,distanceXR);
	printf("Input Parameters: BG sources: NBG = %d | BGLoad = %f | LBG = %d | BG traffic mode = %d\n",NBG,BGLoad,LBG,BG_mode);


	XRWiFisim az;

 	az.Seed=seed;
	az.StopTime(STime);
	az.Setup(NXR,fps,XRLoad,10000,NBG,BGLoad,LBG,BG_mode,distanceXR,RCA);

	printf("Run\n");

	az.Run();

	return 0;
};
