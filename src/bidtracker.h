#ifndef BIDTRACKER_H
#define BIDTRACKER_H

#include "main.h"
#include "wallet.h"
#include "base58.h"
//#include "curl/curl.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include "json/json_spirit.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

using namespace json_spirit;

class Bidtracker 
{
 
public:

    std::string *response;
    std::string *balance;
    CURL *curl;
    std::string url;
	void getunspent();
	CAmount getbalance();
	const mValue& getPairValue(const mObject& obj, const std::string& name);
	void getsender();


};
#endif // BIDTRACKER_H
