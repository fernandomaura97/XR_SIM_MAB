
#ifndef _PHY_model_
#define _PHY_model_

#include <math.h>

class PHY_model {    
	public:             
    		double CalculateDistance(double x1, double y1,double z1,double x2,double y2,double z2);
		double PathLoss(double d);
};

double CalculateDistance(double x1, double y1,double z1,double x2,double y2,double z2)
{
	double d = pow(x1-x2,2) + pow(y1-y2,2) + pow(z1-z2,2);
	d = pow(d,0.5);
	return(d); 
};

double PathLoss(double d)
{
	//	float gamma = 4.4;
	//	double PL = 25 + 10*gamma*log10(d);

	// TMB model

	float gamma = 2.06067;
	double PL = 54.12 + 10*gamma*log10(d)+5.25*0.1467*d;

	return PL;
};

/*
double MCS(double d)
{
	float gamma = 2.4;
	double PL = 25 + 10*gamma*log10(d);
	

};
*/

#endif
