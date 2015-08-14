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



class Bidtracker 
{ 
public:

    CURL *curl; 
	std::string btcgetunspent();
	std::string ltcgetunspent();
	std::string dashgetunspent();
	std::string bcrgetunspent();
	std::string getbids(int nHeight);
	std::string getunspent();
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
