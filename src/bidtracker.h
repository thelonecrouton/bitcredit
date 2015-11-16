#ifndef BIDTRACKER_H
#define BIDTRACKER_H

#include "main.h"

#include <curl/curl.h>
#include <iostream>
#include <string>
#include "json/json_spirit.h"

using namespace json_spirit;

#ifndef BOOST_SPIRIT_THREADSAFE
#define BOOST_SPIRIT_THREADSAFE
#endif

void getbids();

extern int totalbid;
extern std::map<std::string,double> getbidtracker();

class Bidtracker
{
public:

      CURLcode res;
    CURL *curl;
	void btcgetunspent();
	void btcgetunspentbackup();
	void btcsortunspent();
	void btcsortunspentbackup();
	double getbalance(std::string url, double balance);
	double usdbtc();
	long double bcrbtc();
	double btcgetprice();
	double bcrgetprice();
	double credit();
	double newcredit;
	double totalcredit;
	void combine();
};


#endif // BIDTRACKER_H
