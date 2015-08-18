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

std::string ltcbidsurl = "http://ltc.blockr.io/api/v1/address/balance/LS18wropMM8VwT8YFiEZJE7LK8UdZVXXyE";
std::string bcrreservesurl ="http://chainz.cryptoid.info/bcr/api.dws?q=getbalance&a=5qH4yHaaaRuX1qKCZdUHXNJdesssNQcUct";
std::string btcbidsurl ="https://blockchain.info/q/addressbalance/19CMyEEXUoNrpJEipadqmmpZqznWsRc9RV";
std::string dashbidsurl ="http://chainz.cryptoid.info/dash/api.dws?q=getbalance&a=Xj4XjWytC6Etax46JDgwJLPxLSA3XdaQ1C";
std::string ltcreservesurl = "http://ltc.blockr.io/api/v1/address/balance/LS18wropMM8VwT8YFiEZJE7LK8UdZVXXyE";
std::string btcreservesurl = "https://blockchain.info/q/addressbalance/3AsVTW5W5NL8bx1twwR7TabFAYGKstizjt";
std::string dashreservesurl = "http://chainz.cryptoid.info/dash/api.dws?q=getbalance&a=Xj4XjWytC6Etax46JDgwJLPxLSA3XdaQ1C"; 
Bidtracker r;

double Rawdata::_btcbids()
{
	
double btcbids= r.getbalance(btcbidsurl, 0);	
	
 return btcbids; 	
}

double Rawdata::_ltcbids(){

double ltcbids= r.getbalance(ltcbidsurl, 0);

return ltcbids;
}

double Rawdata::_dashbids(){

double dashbids= r.getbalance(btcbidsurl, 0);	

return dashbids;	
}

double Rawdata::_bcrreserves(){
	
double bcrreserves= r.getbalance(bcrreservesurl, 0);	

return bcrreserves; 	
}

double Rawdata::_btcreserves()
{ 

double btcreserves= r.getbalance(btcreservesurl, 0);	

return btcreserves;	
}

double Rawdata::_ltcreserves(){
	
double ltcreserves= r.getbalance(ltcreservesurl, 0);

return ltcreserves;
}


double Rawdata::_dashreserves(){
	
double dashreserves= r.getbalance(dashreservesurl, 0);

return dashreserves;
}

double Rawdata::credit(){
	   
	double x = _bcrreserves() * r.bcrbtc();
	double y = _ltcreserves() * r.ltcbtc();
	double z = _dashreserves() * r.dashbtc();
	double m =(_btcreserves())/COIN;
	double l = x + y + z + m;
	l*=r.usdbtc();

	return l;
}

double Rawdata::reserves(){
	   
	double x = _bcrreserves() * r.bcrbtc();
	double y = _ltcreserves() * r.ltcbtc();
	double z = _dashreserves() * r.dashbtc();
	double m =(_btcreserves())/COIN;
	double l = x + y + z + m;
	
	return l;
}

double Rawdata::newcredit()
{
 return ((_btcbids()/COIN) * r.usdbtc()) + ((_ltcbids() * r.ltcbtc())*r.usdbtc()) + ((_dashbids() * r.dashbtc())*r.usdbtc());	
}

double Rawdata::totalbids(){
	
return (_btcbids()/COIN) + (_ltcbids() * r.ltcbtc()) + (_dashbids() * r.dashbtc());
}

double Rawdata::totalcredit(){
return 	credit() + newcredit();
}


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

