#ifndef BIDTRACKER_H
#define BIDTRACKER_H

#include "main.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include "json/json_spirit.h"

#ifndef BOOST_SPIRIT_THREADSAFE
#define BOOST_SPIRIT_THREADSAFE
#endif
void getbids();
extern int totalbid;
extern std::map<std::string,double> getbidtracker();
class Bidtracker 
{ 
public:

    CURL *curl; 
	void btcgetunspent();
	void ltcgetunspent();
	void dashgetunspent();
	double getbalance(std::string url, double balance);
	double usdbtc(); 
	long double ltcbtc();
	long double dashbtc();
	long double bcrbtc();
	double credit(); 
	double newcredit;
	double totalcredit;
	void combine();    	
};

#endif // BIDTRACKER_H
