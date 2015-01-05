#ifndef STATS_H
#define STATS_H

#include <iostream>
#include <QObject>
#include <math.h>
#include "util.h"
#include "init.h"
#include "wallet.h"
#include "main.h"
#include "coins.h"
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

	CAmount moneysupply()
	{
		CCoinsStats ss;
		FlushStateToDisk();
		
		if (pcoinsTip->GetStats(ss)) 
		{
		CAmount x =ss.nTotalAmount/100000000;
		return x;
		}
		
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
	

	int64_t balance() 
	{
		int64_t bal = pwalletMain->GetBalance();
	
		return bal;
	}
    
	double Gettrust()
	{
		double trust=0;
			{		
				{
					// lifetime carries up to 10 %
					if (lifetime() < month)
						trust+= 0.0;
					
					if (lifetime() > month && lifetime() < 2 * month )
						trust+= 0.01;	
					
					if (lifetime() > 2* month && lifetime() < 3 * month )
						trust+= 0.03;
					
					if (lifetime() > 3 * month && lifetime() < 6 * month )
						trust+= 0.05;
					
					if (lifetime() > 6* month && lifetime() < 12 * month )
						trust+= 0.07;
					
					if (lifetime() > 12 * month)
						trust+= 0.1;
					
				}	
				
				{//wallet useage up to 5%
					if (pwalletMain->mapWallet.size()<10)
						trust+= 0.0001;
						
					if (pwalletMain->mapWallet.size()>10 && pwalletMain->mapWallet.size()<100)
						trust+= 0.01;	
						
					if (pwalletMain->mapWallet.size()>100 && pwalletMain->mapWallet.size()<250)
						trust+= 0.02;
						
					if (pwalletMain->mapWallet.size()>250 && pwalletMain->mapWallet.size()<500)
						trust+= 0.03;
						
					if (pwalletMain->mapWallet.size()>1000)
						trust+= 0.05;	
					
				}
				
				{//wallet balance up to 10%
	
					if(balance() < 1000)
						trust+= 0.001;
				
					if(balance() > 1000 && balance() < 1500)
						trust+= 0.01;
				
					if(balance() >1500 && balance() < 3000)
						trust+= 0.02;
				
					if(balance() >3000 && balance() < 5000)
						trust+= 0.04;
				
					if(balance() >5000 && balance() < 10000)
						trust+= 0.06;
				
					if(balance() >10000)
						trust+= 0.1;		
				
				}
				
				{	//wallet balance up to 10%	
	
					/*if (rmtxincount()> 0.1 && rmtxincount()< 0.15 ) 
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
						trust+= 0.5;*/
				}
				
				{ //network tx participation up to 10%
					if (networktxpart()< 0.00001)
						trust+=0.01;
					if (networktxpart()> 0.00001 && networktxpart()< 0.00005)
						trust+=0.02;
					if (networktxpart()> 0.00005 && networktxpart()< 0.0001)
						trust+=0.04;
					if (networktxpart()> 0.0001 && networktxpart()< 0.001)
						trust+=0.06;
					if (networktxpart()> 0.001)
						trust+=0.1;	
				}	
			
				/*{//is ICO or founder account 25%
					if (Isfdraccount)
						trust += 0.25;
				}*/
				
			}
	}

	double Getmintrust()
	{
		double  x= moneysupply()/lifetime();
	
		return x; 	 
	}

	double creditscore()
	{
		return Gettrust();
	}

	double Getmincreditscore() const
	{
		return 5.5; //static for now need to figure out a way to make it dynamic 
	}

	double Getglbcreditscore()
	{
		return 100;//static for now need to figure out a way to make it dynamic

	}

	double Getavecreditscore()
	{
		return Getglbcreditscore()/Getmincreditscore();

	}

	double Getbestcreditscore()
	{
		return 10;

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

	double Getgrantsaverage()
	{
		return 0;
	}

	int64_t Getbankreserve()
	{
		return 0;
	}

	int64_t Getgblavailablecredit() 
	{
		
		return (moneysupply());
	}

	int64_t Getglobaldebt()
	{
		return 0;
	}

	double Getgblmoneysupply()
	{
		return moneysupply();
	}
	
};

#endif
