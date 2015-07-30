//Copyrights, no copyrights for anyone, copy this and use it outside Bitcredits:- you pay me 100 BTC , hope that's clear enough
//property of The Author aka Minato aka bitcreditscc

#include "bankmath.h"

#include <iostream>
#include <math.h>
#include "activebanknode.h"

CAmount Bankmath::Getgblavailablecredit() 
{
	Rawdata data;
	CAmount n = data.getltcreserves();
	return n;
}

int64_t Bankmath::Getglobaldebt()
{
	Rawdata data;
	CAmount n = data.getltcreserves() + data.Getgrantstotal() ; //representing how much is available for public lending  
	
	return n;
}

double Bankmath::savefactor()
{ 
	Rawdata data;
	double m = 	data.incomingtx()/data.totalnumtx();
		return m;
}

double Bankmath::spendfactor()
{
	Rawdata data;
	double m = 	data.outgoingtx()/data.totalnumtx();
		return m;
}


double Bankmath::txfactor ()
{
	Rawdata data;
	double m = 	data.incomingtx()/data.outgoingtx();
	return m; 
}

double  Bankmath::nettxratio ()
{
	Rawdata data;
	double g =data.getNumTransactions()/data.totalnumtx();
	return g;
}

CAmount Bankmath::stake()
{
	Rawdata data;
	CAmount d =data.balance()/data.Getgblmoneysupply();
	return d;
}

double Bankmath::stage ()
{
		
	if (chainActive.Tip()->nHeight<105000)
		return 3;
		
	if (chainActive.Tip()->nHeight<210000)
		return 4.5;
		
	if (chainActive.Tip()->nHeight<315000)
		return 6;
		
	if (chainActive.Tip()->nHeight<420000)
		return 7.5;
		
	if (chainActive.Tip()->nHeight<425000)
		return 9;
	
	if (chainActive.Tip()->nHeight>530000)
		return 10.0;		
		
	return 0;
}
    
double Bankmath::Gettrust()
{
	double trust=0;
		{	
			Rawdata data;
			int onemonth = data.onemonth;
			double lifetime = data.lifetime();	
			{
				// lifetime carries up to 15 
				
				if (lifetime > 12 * onemonth )
					trust+= 15;	
				
				else if (lifetime > 3* onemonth && lifetime < 12 * onemonth )
					trust+= lifetime/onemonth;
				
				else 
					trust+= 0;
				
			}	
				
			{//wallet useage up to 15
				
									
				if (pwalletMain->mapWallet.size()>10000){
					trust+= 15;
				}
				else if (pwalletMain->mapWallet.size()>0 && pwalletMain->mapWallet.size()< 10000){
					trust+= pwalletMain->mapWallet.size()/1000;
				}
				else 
					trust+= 0;		

			}
				
			{//wallet balance up to 15
				CAmount balance = data.balance(); 
				
				if(balance > 1000000){
					trust+= 15;
				}
				else if(balance > 0 && balance < 1000000){
					trust+= balance/100000;
				}	
				else 
					trust+= 0;		
			
			}
				
				
			{ //network tx participation up to 15
				double networktxpart = data.networktxpart();
				if (networktxpart> 0.01)
					trust+=15;
				else if (networktxpart> 0.0001 && networktxpart< 0.01)
					trust+= networktxpart *1000;
				else 
					trust+= 0;	
				

			}	
			
			{//is BN 20
				if(activeBanknode.status == BANKNODE_IS_CAPABLE)
					trust += 20;
			}
			
			{ //network freq up to 30
				CCoinsStats stats;
				double freq = (data.getNumTransactions()/ (data.lifetime()/data.oneday));
				double gblfreq = (stats.nTransactions/ (data.gbllifetime()/data.oneday));
				{
					if (freq> 100)
					trust+=15;
				else if (freq> 0.0001 && freq< 100)
					trust+= freq/10;
				else 
					trust+= 0;	
					
				}
				
				double relfreq =freq/gblfreq;	
					
				{
				if (relfreq> 0.01)
					trust+=15;
				else if (relfreq> 0.00001 && relfreq< 0.01)
					trust+= relfreq*100;
				else 
					trust+= 0;	
					
				}
					
			}
			
			{ //network stake 15
				
				double stake = data.balance()/data.Getgblmoneysupply();
				if (stake> 0.01)
					trust+=15;
				else if (stake> 0.0001 && stake< 0.01)
					trust+= stake *1000;
				else 
					trust+= 0;	
			}							
		}
		return trust;
}

double Bankmath::Getmintrust()
{
	double  x= 0.31;
	
	return x + stage(); 	 
}

double Bankmath::creditscore()
{
	return Gettrust(); //unforntunately static as well since i have to build a transaction tracking system
}

double Bankmath::Getmincreditscore() const
{
	return 0.5; 
}

double Bankmath::Getavecreditscore()
{
	return Getmincreditscore();

}

double Bankmath::Getavetrust()
{
	return Gettrust();
}

double Bankmath::Getgrossinterestrate()
{
	return 0; //Disabled until PoS
}

double Bankmath::Getnetinterestrate()
{
	return 0; //disabled until PoS
}
    
CAmount Bankmath::moneysupply()
{
	CCoinsStats ss;
	FlushStateToDisk();
	CAmount x =ss.nTotalAmount/COIN;
	Rawdata data;
	return x;
}

double Bankmath::Getinflationrate()
{
	CCoinsStats ss;
	CAmount x =ss.nTotalAmount/COIN;
	double m = 45000/x;
	return m;
}
