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
	
	int onehour = 3600;
	
	int oneday = onehour *24;
	
	int oneweek = oneday * 7;
	
	int onemonth = oneweek *4;
	
	int oneyear = onemonth *12;
  
	int totalnumtx();  //total number of chain transactions
	
	int incomingtx();
	
	int outgoingtx();
	
	int getNumTransactions() const ;//number of wallet transactions
	
	bool verifynumtx ();
	
	int netheight ();
	
	CAmount blockreward (int nHeight, CAmount& nFees );
	
	CAmount	banksubsidy (int nHeight, CAmount& nFees );
	
	CAmount	votesubsidy (int nHeight, CAmount& nFees );
	
	double networktxpart(); //wallet's network participation
	
	double lifetime(); //wallet's lifetime

	int gbllifetime(); //blockchain lifetime
	
	CAmount balance(); 
	
	CAmount Getbankreserve();
	
	CAmount Getbankbalance();
	
	CAmount Getgrantbalance();
	
	CAmount Getescrowbalance(); 

	CAmount Getgrantstotal();

	CAmount Getgblavailablecredit(); 

	CAmount Getglobaldebt();

	CAmount Getgblmoneysupply();

};

#endif //RAWDATA_H
