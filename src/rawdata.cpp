#include "rawdata.h"

#include <iostream>
#include <math.h>
#include <string>
#include "util.h"
#include "init.h"
#include "wallet.h"
#include "main.h"
#include "coins.h"
#include "rpcserver.h"

int onehour = 3600;
	
int oneday = onehour *24;
	
int oneweek = oneday * 7;
	
int onemonth = oneweek *4;
	
int oneyear = onemonth *12;

int Rawdata::totalnumtx()  //total number of chain transactions
{
	int ttnmtx = chainActive.Tip()->nChainTx;

	return ttnmtx;
} 

int Rawdata::incomingtx()  //total number of incoming transactions
{
	int ttnmtx = 0;

	return ttnmtx;
} 

int Rawdata::outgoingtx()  //total number of outgoing transactions
{
	int ttnmtx = 0;
	
	return ttnmtx;
} 

int Rawdata::getNumTransactions() const //number of wallet transactions
{
	//total number of transactions for the account
	int numTransactions = pwalletMain->mapWallet.size();
		return numTransactions; 
}

bool Rawdata::verifynumtx ()
{
	
		return false;
}


int Rawdata::netheight ()
{
	return chainActive.Height();
}

double Rawdata::networktxpart() //wallet's network participation
{
	double netpart = pwalletMain->mapWallet.size()/chainActive.Tip()->nChainTx;

	return netpart;
	
}

double Rawdata::lifetime() //wallet's lifetime 
{
	int creationdate  = pwalletMain->GetOldestKeyPoolTime();
	int lifespan = (GetTime() - creationdate);
    
	return lifespan;
}

int Rawdata::gbllifetime() //blockchain lifetime in days
{
	int a  = GetTime() - 1418504572;
             
	return a/86400;
}

int64_t Rawdata::balance() 
{
	int64_t bal = pwalletMain->GetBalance()/COIN;

	return bal;
}
	
CAmount Rawdata::getltcreserves() 
{
	Bidtracker r;
	CAmount reserve = r.ltcgetbalance();
	return reserve;
}

CAmount Rawdata::getbtcreserves() 
{
	Bidtracker r;	
	CAmount reserve = r.btcgetbalance();
	return reserve;
}

CAmount Rawdata::getbcrreserves() 
{
	Bidtracker r;
	CAmount reserve = r.bcrgetbalance();
	return reserve;
}

CAmount Rawdata::getdashreserves() 
{
	Bidtracker r;
	CAmount reserve = r.dashgetbalance();
	return reserve;
}

CAmount Rawdata::Getgblmoneysupply()
{
		CCoinsStats ss;
		FlushStateToDisk();
		
		if (pcoinsTip->GetStats(ss)) 
		{
		CAmount x =ss.nTotalAmount/COIN;
		return x;
		}
		return 0;
	
}

CAmount Rawdata::Getgrantstotal()
{
	int blocks = chainActive.Height() - 40000;
	int totalgr = blocks * 10;
	return totalgr;  
}

