#ifndef BIDTRACKER_H
#define BIDTRACKER_H

#include "main.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include "json/json_spirit.h"

using namespace json_spirit;

extern double _btcbids;
extern double _ltcbids;
extern double _dashbids;
extern double _bcrreserves;
extern double _btcreserves;
extern double _ltcreserves;
extern double _dashreserves;
extern double usdbtc; 
extern double ltcbtc;
extern double dashbtc;
extern double bcrbtc;
extern double credit(); 
extern double newcredit;
extern double totalcredit;

class Bidtracker 
{ 
public:

    CURL *curl; 
	std::string btcgetunspent();
	std::string ltcgetunspent();
	std::string dashgetunspent();
	std::string bcrgetunspent();
	std::string getbids(int nHeight);		
};

#endif // BIDTRACKER_H
