#ifndef BANKMATH_H
#define BANKMATH_H

#include <iostream>
#include "coins.h"
#include "rawdata.h"
using namespace std;

class Bankmath
{
public:
		
	CAmount Getglobaldebt();
	
  	double savefactor();
  	
  	double stage();
  	
  	CAmount stake(); 

	double spendfactor();

	int rmtxincount();

	double txfactor ();

	double spendratio();

	double saveratio();

	double nettxratio ();
  
	double creditscore();

	double Getmincreditscore() const;

	double Getavecreditscore();

	double Getavetrust();
	
	CAmount moneysupply();

	double Getgrossinterestrate();

	double Getnetinterestrate();

	double Getinflationrate();
	
	double Gettrust();

	double Getmintrust();	  

	double gblavailablecredit();
	
};


#endif
