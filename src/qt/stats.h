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

<<<<<<< HEAD
=======
	double blockreward (int nHeight, CAmount& nFees )
	{
		double x = GetBlockValue(nHeight, nFees);	
			return x;
	}
	
	
	

	double stage ()
	{
		int nHeight;
		if (chainActive.Tip()->nHeight<8000)
			return 0.0;
			
		if (chainActive.Tip()->nHeight<15000)
			return 1.0;
			
		if (chainActive.Tip()->nHeight<40000)
			return 1.3;
			
		if (chainActive.Tip()->nHeight<60000)
			return 2.0;
			
		if (chainActive.Tip()->nHeight<80000)
			return 4.0;
			
		if (chainActive.Tip()->nHeight>100000)
			return 10.0;				
	}



>>>>>>> origin/master2
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
<<<<<<< HEAD
       
		return a;
=======
              
		return a/3600;
>>>>>>> origin/master2
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
<<<<<<< HEAD
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
=======
					// lifetime carries up to 20 %
					if (lifetime() < 3* month)
						trust+= 0.1;
					
					if (lifetime() > 3* month && lifetime() < 4 * month )
						trust+= 0.3;	
					
					if (lifetime() > 4* month && lifetime() < 5 * month )
						trust+= 0.5;
					
					if (lifetime() > 5 * month && lifetime() < 6 * month )
						trust+= 0.1;
					
					if (lifetime() > 6* month && lifetime() < 12 * month )
						trust+= 0.15;
					
					if (lifetime() > 12 * month)
						trust+= 0.20;
					
				}	
				
				{//wallet useage up to 20%
					if (pwalletMain->mapWallet.size()<10)
						trust+= 0.01;
						
					if (pwalletMain->mapWallet.size()>10 && pwalletMain->mapWallet.size()<100)
						trust+= 0.02;	
						
					if (pwalletMain->mapWallet.size()>100 && pwalletMain->mapWallet.size()<250)
						trust+= 0.05;
						
					if (pwalletMain->mapWallet.size()>250 && pwalletMain->mapWallet.size()<500)
						trust+= 0.1;
						
					if (pwalletMain->mapWallet.size()>1000)
						trust+= 0.2;	
					
				}
				
				{//wallet balance up to 20%
	
					if(balance() < 1000)
						trust+= 0.01;
				
					if(balance() > 1000 && balance() < 1500)
						trust+= 0.02;
				
					if(balance() >1500 && balance() < 3000)
						trust+= 0.04;
				
					if(balance() >3000 && balance() < 5000)
						trust+= 0.06;
				
					if(balance() >5000 && balance() < 10000)
						trust+= 0.08;
				
					if(balance() >100000)
						trust+= 0.2;		
				
				}
				
				
				{ //network tx participation up to 20%
					if (networktxpart()< 0.00001)
						trust+=0.01;
					if (networktxpart()> 0.00001 && networktxpart()< 0.00005)
						trust+=0.03;
					if (networktxpart()> 0.00005 && networktxpart()< 0.0001)
						trust+=0.05;
					if (networktxpart()> 0.0001 && networktxpart()< 0.001)
						trust+=0.1;
					if (networktxpart()> 0.001)
						trust+=0.2;	
				}	
			
				/*{//is ICO or founder account 30% 
					if (Isfdraccount)
						trust += 0.30;
>>>>>>> origin/master2
				}*/
				
			}
	}

	double Getmintrust()
	{
<<<<<<< HEAD
		double  x= moneysupply()/lifetime();
		 
		return x; 	 
=======
		double  x= 0.31;
		
		return x + stage(); 	 
>>>>>>> origin/master2
	}

	double creditscore()
	{
<<<<<<< HEAD
		return Gettrust();
=======
		return Gettrust(); //unforntunately static as well since i have to build a transaction tracking system
>>>>>>> origin/master2
	}

	double Getmincreditscore() const
	{
<<<<<<< HEAD
		return 5.5; //static for now need to figure out a way to make it dynamic 
	}

	double Getglbcreditscore()
	{
		return 100;//static for now need to figure out a way to make it dynamic

=======
		return 0.5; 
>>>>>>> origin/master2
	}

	double Getavecreditscore()
	{
<<<<<<< HEAD
		return Getglbcreditscore()/Getmincreditscore();

	}

	double Getbestcreditscore()
	{
		return 10;
=======
		return Getmincreditscore();
>>>>>>> origin/master2

	}

	double aveprice()
	{
		double  x=0;
	
		return x; 	 
	}

<<<<<<< HEAD
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
=======
	double Getavetrust()
	{
		return Gettrust();
>>>>>>> origin/master2
	}

	double Getgrossinterestrate()
	{
<<<<<<< HEAD
		return 0;
=======
		return 0; //Disabled until PoS
>>>>>>> origin/master2
	}

	double Getnetinterestrate()
	{
<<<<<<< HEAD
		return 0;
	}

	double Getgblinterestrate()
	{
		return 0.03; //temporarily fixed
	}

	double Getgrantindex()
	{
		return 0.05; //temporarily fixed
	}

	double Getexpectedchange()
	{
		return 0;
=======
		return 0; //disabled until PoS
>>>>>>> origin/master2
	}

	double Getinflationindex()
	{
<<<<<<< HEAD
		return 0;
	}

	double Getconsensusindex()
	{
		return 0;
=======
		double a = moneysupply() - 10000000;
		
		return  a/ moneysupply();
>>>>>>> origin/master2
	}

	int64_t Getvolume()
	{
		return moneysupply() -10000000 ; // need exchange to provide, unless we just consider volume per period within the client
	}

	int64_t Getgrantstotal()
	{
<<<<<<< HEAD
		return 0; //need to retrun balance of specific account
=======
		return (moneysupply()- 10500000) * 0.1; // post v 30.10
>>>>>>> origin/master2
	}

	int64_t Getbankreserve()
	{
<<<<<<< HEAD
		return 0; //need to retrun balance of specific account
=======
		return (moneysupply()- 10500000) * 0.1; // post v 30.10
>>>>>>> origin/master2
	}

	int64_t Getgblavailablecredit() 
	{
		
<<<<<<< HEAD
		return moneysupply() -7000000; //credit available from the bank and the reserve
=======
		return Getbankreserve();
>>>>>>> origin/master2
	}

	int64_t Getglobaldebt()
	{
<<<<<<< HEAD
		return moneysupply()-(Getgrantstotal()+Getbankreserve()); //representing how much of the people's money the bank holds 
=======
		return moneysupply()- Getbankreserve()-Getgrantstotal() ; //representing how much of the people's money the bank holds 
	}

	double Getgrantindex()
	{
		return Getgrantstotal()/moneysupply(); 
>>>>>>> origin/master2
	}

	double Getgblmoneysupply()
	{
		return moneysupply(); //overall money supply 
	}
	
};

#endif
