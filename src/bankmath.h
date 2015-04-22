#ifndef BANKMATH_H
#define BANKMATH_H

#include <iostream>


using namespace std;

class Bankmath
{
public:
  Bankmath() : nEntry(0), X(0), X2(0), Y(0), Y2(0), XY(0) {}
  void Reset() ;
  void EnterData(double nX, double nY);
  
  int GetEntryCount() {return nEntry;}
  double GetMean(bool retY=true);
  double GetVariance(bool retY=true);
  double GetStandardDeviation(bool retY=true);
  double GetCovariance();
  double GetCorrelation();
  
  	bool savefactor();

	double spendfactor();

	int rmtxincount();

	double txfactor ();

	double spendratio();

	double saveratio();

	int nettxratio ();
  
  
  
private:
	int nEntry;
  double X;
  double X2;
  double Y;
  double Y2;
  double XY;
};


#endif
