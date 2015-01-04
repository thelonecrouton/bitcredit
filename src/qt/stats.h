#ifndef STATS_H
#define STATS_H

#include <iostream>
#include <QObject>
#include <math.h>
#include "util.h"
#include "init.h"
#include "wallet.h"
#include "main.h"
#include "rpcserver.h"

using namespace std;

int month = 604800 *4;


class Stats : public QObject
{
  public:
  
	int totalnumtx()  //total number of chain transactions
	{
		int ttnmtx = chainActive.Tip()->nChainTx;
	
		return ttnmtx;
	} 

	int getNumTransactions() const //number of wallet transactions
	{
		//total number of transactions for the account
		int numTransactions = pwalletMain->mapWallet.size();

		return numTransactions; 
	}

	double moneysupply() //money supply generated
	{
		CCoinsStats ss;
		return ss.nTotalAmount;
	}

	double networktxpart() //wallet's network participation
	{
		double netpart = pwalletMain->mapWallet.size()/chainActive.Tip()->nChainTx;
	
		return netpart;
	}

	int lifetime() //wallet's lifetime
	{
		int creationdate  = pwalletMain->GetOldestKeyPoolTime();
		int lifespan = (GetTime() - creationdate)/3600;
    
		return lifespan;
	}

	int gbllifetime() //blockchain lifetime
	{
		int a  = GetTime() - 1418504572;
       
		return a;
	}

	double freq () // transactional frequency
	{
		double a = (getNumTransactions()/ lifetime());
    
		return a;    
	}

	double glbfreq () //networks' transactional frequency 
	{
		double b = (chainActive.Tip()->nChainTx/ gbllifetime() );
    
		return b;    
	}
    
	bool savefactor()
	{ 
		if (pwalletMain->mapWallet.size() < 20)
			return false;
	}

	int rmtxincount()
	{ 
		if(!savefactor())
		return 0;
	
		int txcount = (22)/pwalletMain->mapWallet.size();

		return txcount;
	}

	int64_t balance() 
	{
		int64_t bal = pwalletMain->GetBalance();
	
		return bal;
	}

	double spendfactor()
	{
		int spfactor = totalsnt/totalrvd;
	
		return spfactor;
	}

	double txfactor ()
	{
		double txratio = txrvd/txsnt;
	
		return txratio; 
	}

	double spendratio()
	{
		return	(txsnt/getNumTransactions());
	}

	double saveratio()
	{
		return (txrvd/getNumTransactions());	
	}

	int nettxratio ()
	{
		return getNumTransactions()/totalnumtx();
	}
    
	

	bool trust()
	{
		if (pwalletMain->mapWallet.size()<1)
			return false;

		if (lifetime() < month )
			return false;
	
		if(balance() < 1000)
			return false;
   
	}

	double Gettrust()
	{
		double trust=0;
			{
				/*if (!trust)
					return 0.0;*/ //will reinstate after testing
	
				if (rmtxincount()> 0.1 && rmtxincount()< 0.15 ) 
					trust+= 0.1;
 
				if (rmtxincount()> 0.15 && rmtxincount()< 0.2) 
					trust+= 0.15; 
 
				if (rmtxincount()> 0.2 && rmtxincount()< 0.25) 
					trust+= 0.2;

				if (rmtxincount()> 0.25 && rmtxincount()< 0.3) 
					trust+= 0.25;

				if (rmtxincount()> 0.3 && rmtxincount()< 0.35) 
					trust+= 0.3;

				if (rmtxincount()> 0.35 && rmtxincount()< 0.4) 
					trust+= 0.35;

				if (rmtxincount()> 0.4 && rmtxincount()< 0.45) 
					trust+= 0.4;
 
				if (rmtxincount()> 0.45 && rmtxincount()< 0.5) 
					trust+= 0.45; 

				if (rmtxincount()> 0.5) 
					trust+= 0.5;
			}
	
			{
				if (networktxpart()> 0.001)
					trust+=0.2;
			}
	
			{
				if (spendratio()<saveratio())
					trust += 0.3;
			}
	}

	double Getmintrust()
	{
		double  x= (gbllifetime()/totalnumtx()) * (chainActive.Tip()->nHeight);
	
		return x; 	 
	}

	double creditscore()
	{
		return 0;
	}

	double Getmincreditscore() const
	{
		return 0.1;
	}

	double Getavecreditscore()
	{
		return 0;

	}

	double Getglbcreditscore()
	{
		return 0;

	}

	double Getbestcreditscore()
	{
		return 0;

	}

	double aveprice()
	{
		double  x=0;
	
		return x; 	 
	}

	double Getgbltrust()
	{
		(Getmintrust() * glbfreq () );
	}

	double Getbesttrust()
	{
		Getmintrust() + glbfreq ();
	}

	double Getavetrust()
	{
		(Getgbltrust() + Getbesttrust())/(Getgbltrust() - Getbesttrust());
	}

	double Getgrossinterestrate()
	{
		return 0;
	}

	double Getnetinterestrate()
	{
		return 0;
	}

	double Getgblinterestrate()
	{
		return 0;
	}

	double Getgrantindex()
	{
		return 0;
	}

	double Getexpectedchange()
	{
		return 0;
	}

	double Getinflationindex()
	{
		return 0;
	}

	double Getconsensusindex()
	{
		return 0;
	}

	int64_t Getvolume()
	{
		return 0;
	}

	int64_t Getgrantstotal()
	{
		return 0;
	}

	int64_t Getbankreserve()
	{
		return 0;
	}

	int64_t Getgblavailablecredit() // crude Quantity theory of money
	{
		
		return (moneysupply() / (moneysupply() - Getbankreserve()) - Getgrantstotal() );
	}

	int64_t Getglobaldebt()
	{
		return 0;
	}

};

#endif
