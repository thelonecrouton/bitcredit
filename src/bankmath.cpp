//Copyrights, no copyrights for anyone, copy this and use it outside Bitcredits:- you pay me 100 BTC , hope that's clear enough
//property of The Author aka Minato aka bitcreditscc

#include "bankmath.h"

#include <iostream>
#include <QObject>
#include <math.h>
#include "util.h"
#include "init.h"
#include "wallet.h"
#include "main.h"
#include "coins.h"
#include "rpcserver.h"

int64_t Bankmath::Getgblavailablecredit() 
{
	return Getbankreserve();
}

int64_t Bankmath::Getglobaldebt()
{
	return moneysupply()- Getbankreserve()-Getgrantstotal() ; //representing how much of the people's money the bank holds 
}

double stage ()
{
		
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
		
	return 0;
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
    
double Gettrust()
{
	double trust=0;
		{		
			{
				// lifetime carries up to 20 %
				if (lifetime() < 3* onemonth)
					trust+= 0.1;
				
				if (lifetime() > 3* onemonth && lifetime() < 4 * onemonth )
					trust+= 0.3;	
				
				if (lifetime() > 4* onemonth && lifetime() < 5 * onemonth )
					trust+= 0.5;
				
				if (lifetime() > 5 * onemonth && lifetime() < 6 * onemonth )
					trust+= 0.1;
				
				if (lifetime() > 6* onemonth && lifetime() < 12 * onemonth )
					trust+= 0.15;
				
				if (lifetime() > 12 * onemonth)
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
			}*/
			
		}
		return trust;
}

double Getmintrust()
{
	double  x= 0.31;
	
	return x + stage(); 	 
}

double creditscore()
{
	return Gettrust(); //unforntunately static as well since i have to build a transaction tracking system
}

double Getmincreditscore() const
{
	return 0.5; 
}

double Getavecreditscore()
{
	return Getmincreditscore();

}

double Getavetrust()
{
	return Gettrust();
}

double Getgrossinterestrate()
{
	return 0; //Disabled until PoS
}

double Getnetinterestrate()
{
	return 0; //disabled until PoS
}

double Getinflationindex()
{
	double a = moneysupply() - 10000000;
	
	return  a/ moneysupply();
}

int Bankstat::totalnumtx()
{
int ttnmtx = chainActive.Tip()->nChainTx;
	return ttntx;
} 

extern int txrvd, txsnt, totalrvd, totalsnt;

double freq ()
{
  double txfreq= (getNumTransactions()/ lifetime());
    
  return txfreq;    
}

double glbfreq ()
{
  double txfreq= (chainActive.Tip()->nChainTx/ gbllifetime() );
    
   return txfreq;    
}
    
bool savefactor()
{ 
	if ((txrvd - txsnt) < 2)
		return false;
}

double spendfactor()
{
	int spfactor = totalsnt/totalrvd;
	
	return spfactor;
}

int rmtxincount()
{ 
	if(!savefactor())
	return 0;
	
	int txcount = (txrvd - txsnt)/pwalletMain->mapWallet.size();

	return txcount;
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
	getNumTransactions()/totalnumtx();
}

void Bankmath::Reset()
{
  nEntry=0;
  X=0;
  X2=0;
  Y=0;
  Y2=0;
  XY=0;
}

void Bankmath::EnterData(double nX, double nY)
{
  nEntry++;
  X += nX;
  Y += nY;
  X2 += nX*nX;
  Y2 += nY*nY;
  XY += nX*nY;
}

double Bankmath::GetMean(bool retY)
{
  if(retY)
    return (nEntry>0) ? Y/nEntry : 0.0;
  else
    return (nEntry>0) ? X/nEntry : 0.0;
}

double Bankmath::GetVariance(bool retY)
{
  if(retY)
    return (nEntry>1) ? (Y2-(Y*Y/nEntry))/(nEntry-1) : 0.0;
  else
    return (nEntry>1) ? (X2-(X*X/nEntry))/(nEntry-1) : 0.0;
}

double Bankmath::GetStandardDeviation(bool retY)
{
  if(retY)
    return sqrt(GetVariance(true));
  else
    return sqrt(GetVariance(false));
}

double Bankmath::GetCovariance()
{
  return (nEntry>1) ? (XY-nEntry*GetMean(true)*GetMean(false))/(nEntry-1) : -2.0;
}

double Bankmath::GetCorrelation()
{
  return GetCovariance()/(GetStandardDeviation(true)*GetStandardDeviation(false));
}
