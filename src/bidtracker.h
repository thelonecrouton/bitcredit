#ifndef BIDTRACKER_H
#define BIDTRACKER_H

#include "main.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include "json/json_spirit.h"

using namespace json_spirit;

class Bidtracker 
{ 
public:

    std::string *response;
    std::string *balance;
    CURL *curl; 
	std::string btcgetunspent();
	std::string ltcgetunspent();
	std::string dashgetunspent();
	std::string bcrgetunspent();
	double btcgetprice();
	CAmount btcgetbalance();
	CAmount ltcgetbalance();
	CAmount dashgetbalance();
	CAmount fiatgetbalance();
	CAmount bcrgetbalance();
	const mValue& getPairValue(const mObject& obj, const std::string& name);
	

};

#endif // BIDTRACKER_H
