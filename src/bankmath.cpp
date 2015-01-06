#include "bankmath.h"
#include <math.h>

int incomingtx;
int outgoingtx;
int hour = 3600;
int day = 86400;
int week = 604800;
int month = 604800 *4;
int year = 31556926;
int firsttxtime, lasttxtime;

int Bankstat::totalnumtx()
{
	int ttnmtx = chainActive.Tip()->nChainTx;
	
	return ttntx;
} 

extern int txrvd, txsnt, totalrvd, totalsnt;

int Bankstat::getNumTransactions() const
{
	//total number of transactions for the account
    int numTransactions = pwalletMain->mapWallet.size();

	return numTransactions; 
}

double moneysupply()
{
	return chainActive.Tip()->nMoneySupply;
}

double networktxpart()
{
	double netpart = pwalletMain->mapWallet.size()/chainActive.Tip()->nChainTx;
	
	return netpart;
}

int Bankstat::lifetime()
{
    int creationdate  = pwalletMain->GetOldestKeyPoolTime();
    int lifespan = (GetTime() - creationdate)/3600;
    
    return lifespan;
}

int Bankstat::gbllifetime()
{
    int a  = GetTime() - genesis.time;
       
    return a;
}

double Bankstat::freq ()
{
    txfreq= (getNumTransactions()/ nlifetime);
    
    return txfreq;    
}

double Bankstat::glbfreq ()
{
    txfreq= (chainActive.Tip()->nChainTx/ genesis.time );
    
    return txfreq;    
}
    
bool Bankstat::savefactor()
{ 
	if ((txrvd - txsnt) < 2)
		return false;
}



int Bankstat::rmtxincount()
{ 
	if(!savefactor())
	return 0;
	
	int txcount = (txrvd - txsnt)/pwalletMain->mapWallet.size();

	return txcount;
}

int64_t Bankstat::balance() 
(
	int64_t bal = getBalance();
	
	return bal;
)

double Bankstat::spendfactor()
{
	int spfactor = totalsnt/totalrvd;
	
	return spfactor;
}

double Bankstat::txfactor ()
{
	double txratio = txrvd/txsnt;
	return txratio; 
}

double Bankstat::spendratio()
{
	return	(txsnt/getNumTransactions());
}

double Bankstat::saveratio()
{
  return (txrvd/getNumTransactions());	
}

int Bankstat::nettxratio ()
{
	getNumTransactions()totalnumtx();
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
