#ifndef RAWDATA_H
#define RAWDATA_H

#include <iostream>
#include <math.h>
#include "util.h"
#include "init.h"
#include "wallet.h"
#include "main.h"
#include "coins.h"
#include "rpcserver.h"


using namespace std;

class Rawdata 
{
  public:
  
	int totalnumtx();  //total number of chain transactions
	
	int getNumTransactions() const ;//number of wallet transactions
	
	CAmount moneysupply();
	
	CAmount blockreward (int nHeight, CAmount& nFees );
	
	CAmount	banksubsidy (int nHeight, CAmount& nFees );
	
	CAmount	votesubsidy (int nHeight, CAmount& nFees );
	
	double networktxpart(); //wallet's network participation
	
	int lifetime(); //wallet's lifetime

	int gbllifetime(); //blockchain lifetime

	double freq (); // transactional frequency

	double glbfreq (); //networks' transactional frequency 
	
	CAmount balance(); 
    
	double Gettrust();

	double Getmintrust();

	double creditscore();

	double Getmincreditscore() const;

	double Getavecreditscore();

	double Getavetrust();

	double Getgrossinterestrate();

	double Getnetinterestrate();

	double Getinflationindex();

	int64_t Getvolume();
	
	CAmount Getbankreserve();
	
	CAmount Getbankbalance();
	
	CAmount Getgrantbalance();
	
	CAmount Getescrowbalance(); 

	int64_t Getgrantstotal();

	int64_t Getgblavailablecredit(); 

	int64_t Getglobaldebt();

	double Getgblmoneysupply();


	
};

#endif RAWDATA_H
