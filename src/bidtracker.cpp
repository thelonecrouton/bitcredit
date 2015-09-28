#include "bidtracker.h"
#include "banknodeman.h"
#include "wallet.h"
#include "base58.h"
#include "rpcclient.h"
#include "util.h"
#include "json_spirit_reader.h"
#include <iostream>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <sstream>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string remove(string input, char m)
{
  input.erase(std::remove(input.begin(),input.end(), m),input.end());
  
  return input;
}

std::string replacestring(std::string subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

double Bidtracker::getbalance(std::string url, double balance)
{
    const char * c = url.c_str();
    CURL *curl; 
      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }
      
      std::string response = readBuffer;
      if ( ! (istringstream(response) >> balance) ) balance = 0;
	   
      return balance;
}

void btcsortunspent(){

	ifstream myfile ((GetDataDir()/ "bidtracker/btcunspentraw.dat").string().c_str());
	std::ofstream myfile2;
	myfile2.open((GetDataDir()/ "bidtracker/btcbids.dat").string().c_str(),fstream::out);

	std::string line, txid, url;
    try
    {

	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			std::string search3;
			size_t pos;
			size_t f = line.find("			tx_hash_big_endian:");
			size_t g = line.find("			value:");
	
			search = "tx_hash_big_endian";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("			tx_hash_big_endian:").length(), "");
				semp = remove(semp, ',');
				txid = semp;
				url = "https://blockchain.info/rawtx/"+ txid ;
				const char * d = url.c_str();
				CURL *curl;
				CURLcode res;
				string readBuffer;
				curl = curl_easy_init();
				if(curl) {        
					curl_easy_setopt(curl, CURLOPT_URL, d);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					res = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
				}
				std::size_t pos1 = readBuffer.find("value");
				readBuffer = readBuffer.substr(0,pos1);	
				readBuffer = remove(readBuffer, '"');
				readBuffer = remove(readBuffer, '{');
				readBuffer = remove(readBuffer,'}');
				readBuffer = remove(readBuffer, '[');
				readBuffer = remove(readBuffer, '\n');				
				std::string uemp =readBuffer;
				std::size_t pos2 = uemp.find("addr:");
				uemp = uemp.substr(pos2);
				uemp = replacestring(uemp, "addr:", "");
				erase_all(uemp, " ");				
				myfile2 << uemp;
			}

			search2 = "value:";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("			value:").length(), "");
				string value = semp;
				value = remove(value, ',');
				long double amount = atof(value.c_str());
				myfile2 << std::fixed <<  amount << std::endl;				
			}
		}
		myfile.close();
		myfile2.close();
	}
	}	

    catch (std::exception const &exc)
    {
        std::cerr << "Exception caught " << exc.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught\n";
    }

}

std::string Bidtracker::btcgetunspent()
{
    std::string address = "16f5dJd4EHRrQwGGRMczA69qbJYs4msBQ5";
	
    std::string url;
    url = "https://blockchain.info/unspent?active=" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

    boost::filesystem::path biddir = GetDataDir() / "bidtracker";

    if(!(boost::filesystem::exists(biddir))){
        if(fDebug)LogPrintf("Biddir Doesn't Exists\n");

        if (boost::filesystem::create_directory(biddir))
            if(fDebug)LogPrintf("Biddir....Successfully Created !\n");
    }

	const char * d = (GetDataDir().string() + "/bidtracker/btcunspentraw.dat").c_str();
	std::ofstream myfile;
	myfile.open(d,fstream::out);
	readBuffer = remove(readBuffer, ' ');
	readBuffer = remove(readBuffer, '"');
	myfile << readBuffer << std::endl;
	myfile.close();
	return readBuffer;
}

double btcgetprice()
{
	CAmount price; 
    std::string url;
    url = "https://blockchain.info/q/24hrprice";
    
    const char * c = url.c_str();
    CURL *curl; 
      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }
     
      price = atof(readBuffer.c_str());  
      
      return price;
}

double dashgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-DASH";
    
    const char * c = url.c_str();
    CURL *curl;
      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

	std::size_t pos = readBuffer.find(",\"A");
	readBuffer = readBuffer.substr(0,pos);
	readBuffer = replacestring(readBuffer, ",", ",\n");
	readBuffer = remove(readBuffer, ',');
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, ':');
	readBuffer = remove(readBuffer, '{');
	readBuffer = replacestring(readBuffer, "successtrue", "");
	readBuffer = replacestring(readBuffer, "message", "");
	readBuffer = replacestring(readBuffer, "resultBid", "");
	readBuffer = remove(readBuffer, '\n'); 
   if ( ! (istringstream(readBuffer) >> price) ) price = 0;

    return price;
}

double bcrgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-BCR";
    
    const char * c = url.c_str();
    CURL *curl;
      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

	std::size_t pos = readBuffer.find(",\"A");
	readBuffer = readBuffer.substr(0,pos);
	readBuffer = replacestring(readBuffer, ",", ",\n");
	readBuffer = remove(readBuffer, ',');
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, ':');
	readBuffer = remove(readBuffer, '{');
	readBuffer = replacestring(readBuffer, "successtrue", "");
	readBuffer = replacestring(readBuffer, "message", "");
	readBuffer = replacestring(readBuffer, "resultBid", "");
	readBuffer = remove(readBuffer, '\n');
    if ( ! (istringstream(readBuffer) >> price) ) price = 0;

      return price;
}

double ltcgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-LTC";
    
    const char * c = url.c_str();
    CURL *curl;
      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

	std::size_t pos = readBuffer.find(",\"A");
	readBuffer = readBuffer.substr(0,pos);
	readBuffer = replacestring(readBuffer, ",", ",\n");
	readBuffer = remove(readBuffer, ',');
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, ':');
	readBuffer = remove(readBuffer, '{');
	readBuffer = replacestring(readBuffer, "successtrue", "");
	readBuffer = replacestring(readBuffer, "message", "");
	readBuffer = replacestring(readBuffer, "resultBid", "");
	readBuffer = remove(readBuffer, '\n');
	if ( ! (istringstream(readBuffer) >> price) ) price = 0;

    return price;
}

void dashsortunspent(){

	ifstream myfile ((GetDataDir() /"bidtracker/dashunspentraw.dat").string().c_str());
	std::ofstream myfile2;
	myfile2.open((GetDataDir() /"bidtracker/dashbids.dat").string().c_str(),fstream::out);

	std::string line;
    try
    {	
	
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			size_t pos;
			size_t f = line.find("\"id\":");
			size_t g = line.find("\"tx_address_value\":");
	
			search = "\"id\":";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("\"id\":").length(), "");
				semp = remove(semp, '"');
				semp = remove(semp, ',');				
				string txid = semp;
				string url;
				url = "http://api.blockstrap.com/v0/drk/transaction/id/"+ txid + "?showtxn=1&showtxnio=1&";
				const char * d = url.c_str();
				CURL *curl;
				CURLcode res;
				string readBuffer;

				curl = curl_easy_init();
				if(curl) {        
					curl_easy_setopt(curl, CURLOPT_URL, d);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					res = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
				}
			std::size_t pos = readBuffer.find(",{");
			readBuffer = readBuffer.substr(0,pos);
			string hemp = readBuffer;
			std::size_t pos1 = hemp.find("\"address\"");
			hemp = hemp.substr(pos1);

			string lemp = hemp;
			std::size_t pos2 = lemp.find("\"}");
			lemp = lemp.substr(0,pos2);			
			lemp = replacestring(lemp, "\"address\":\"", "");

			myfile2 << lemp << ",";
			}

			search2 = "\"tx_address_value\":";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("\"tx_address_value\":").length(), "");
				semp = remove(semp, ',');				
				long double amount = atof(semp.c_str());
				amount = amount * dashgetprice();
				myfile2 << std::fixed << amount << std::endl;							
			}
		}
		myfile.close();
		myfile2.close();
	}
	}	
    catch (std::exception const &exc)
    {
        std::cerr << "Exception caught " << exc.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught\n";
    }

}

std::string Bidtracker::dashgetunspent()
{
    std::string address = "Xypcx2iE8rCtC3tjw5M8sxpRzn4JuoSaBH";	 
    std::string url;
    url = "http://api.blockstrap.com/v0/drk/address/unspents/" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }
    
	std::ofstream myfile;
	myfile.open((GetDataDir() /"bidtracker/dashunspentraw.dat").string().c_str(),fstream::out);
	std::size_t pos = readBuffer.find("[{");
    readBuffer = readBuffer.substr (pos); 
	readBuffer = replacestring(readBuffer, "},{", "\n},{"); 
	readBuffer = replacestring(readBuffer, ",", ",\n");  
	readBuffer = remove(readBuffer, ' ');	
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer, '}');
	readBuffer = remove(readBuffer, '[');
	myfile << readBuffer << std::endl;
	myfile.close();
	return readBuffer;
} 

std::string Bidtracker::ltcgetunspent()
{
    std::string address = "Lc7ebfQPz6VJ8qmXYaaFxBYLpDz2XsDu7c";	 
    std::string url;
    url = "http://api.blockstrap.com/v0/ltc/address/unspents/" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

	std::ofstream myfile;
	myfile.open((GetDataDir() / "bidtracker/ltcunspentraw.dat").string().c_str(),fstream::out);
	std::size_t pos = readBuffer.find("[{");
    readBuffer = readBuffer.substr (pos); 
	readBuffer = replacestring(readBuffer, "},{", "\n},{"); 
	readBuffer = replacestring(readBuffer, ",", ",\n");  
	readBuffer = remove(readBuffer, ' ');	
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer, '}');
	readBuffer = remove(readBuffer, '[');
	myfile << readBuffer << std::endl;
	myfile.close();
	return readBuffer;
} 

void ltcsortunspent(){

	ifstream myfile ((GetDataDir() / "bidtracker/ltcunspentraw.dat").string().c_str());
	std::ofstream myfile2;
	myfile2.open((GetDataDir() / "bidtracker/ltcbids.dat").string().c_str(),fstream::out);

	std::string line;
    try
    {

	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			size_t pos;
			size_t f = line.find("\"id\":");
			size_t g = line.find("\"tx_address_value\":");
	
			search = "\"id\":";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("\"id\":").length(), "");
				semp = remove(semp, '"');
				semp = remove(semp, ',');				
				string txid = semp;
				string url;
				url = "http://api.blockstrap.com/v0/ltc/transaction/id/"+ txid + "?showtxn=1&showtxnio=1&";
				const char * d = url.c_str();
				CURL *curl;
				CURLcode res;
				string readBuffer;

				curl = curl_easy_init();
				if(curl) {        
					curl_easy_setopt(curl, CURLOPT_URL, d);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					res = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
				}
			std::size_t pos = readBuffer.find(",{");
			readBuffer = readBuffer.substr(0,pos);
			string hemp = readBuffer;
			std::size_t pos1 = hemp.find("\"address\"");
			hemp = hemp.substr(pos1);			

			string lemp = hemp;
			std::size_t pos2 = lemp.find("\"}");
			lemp = lemp.substr(0,pos2);
			
			lemp = replacestring(lemp, "\"address\":\"", "");

			myfile2 << lemp << ",";
			}

			search2 = "\"tx_address_value\":";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("\"tx_address_value\":").length(), "");
				semp = remove(semp, ',');		
				long double amount = atof(semp.c_str());
				amount = amount *ltcgetprice();	
				myfile2 << std::fixed << amount << std::endl;					
			}
		}
		myfile.close();
		myfile2.close();
	}	
	}

    catch (std::exception const &exc)
    {
        std::cerr << "Exception caught " << exc.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught\n";
    }

}

double Bidtracker::usdbtc(){
	
return btcgetprice();	

} 

long double Bidtracker::ltcbtc(){
	
return ltcgetprice();

}

long double Bidtracker::dashbtc(){
	
return dashgetprice();

}
 
long double Bidtracker::bcrbtc(){
	
return bcrgetprice();	

} 

void Bidtracker::combine()
{		
	std::ofstream myfile;
	myfile.open((GetDataDir() /"bidtracker/prefinal.dat").string().c_str(),fstream::out);
	ifstream myfile2((GetDataDir() /"bidtracker/btcbids.dat").string().c_str());
	ifstream myfile3((GetDataDir() /"bidtracker/ltcbids.dat").string().c_str());
	ifstream myfile4((GetDataDir() /"bidtracker/dashbids.dat").string().c_str());
	
	if (myfile2.is_open()){
		std::string line;
		while ( myfile2.good() ){
			getline (myfile2,line);	
	myfile<<line<<endl;
	}	}
	if (myfile3.is_open()){
		std::string line;
		while ( myfile3.good() ){
			getline (myfile3,line);	
	myfile<<line<<endl;
	}	}
	if (myfile4.is_open()){
		std::string line;
		while ( myfile4.good() ){
			getline (myfile4,line);	
	myfile<<line<<endl;
	}	}

	myfile.close();	
	myfile2.close();	
	myfile3.close();	
	myfile4.close();
	remove((GetDataDir() /"bidtracker/btcbids.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/ltcbids.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/dashbids.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/btcunspentraw.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/ltcunspentraw.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/dashunspentraw.dat").string().c_str());		  
}

void cleanfile()
{
	std::ofstream myfile;
	myfile.open((GetDataDir() /"bidtracker/final.dat").string().c_str(),fstream::out);
	ifstream myfile2((GetDataDir() /"bidtracker/prefinal.dat").string().c_str());

	std::string line;
	while ( getline( myfile2, line ) ) {
    if ( ! line.empty() ) {
        myfile << line << '\n';
    }
	}

	myfile.close();
	myfile2.close();
	remove((GetDataDir() /"bidtracker/prefinal.dat").string().c_str());
}

void getbids(){

	int64_t nStart = GetTimeMillis();

	Bidtracker h;
	h.btcgetunspent();
	h.ltcgetunspent();
	h.dashgetunspent();
	ltcsortunspent();
	btcsortunspent();
	dashsortunspent();
	h.combine();
	cleanfile();
	
	if(fDebug)LogPrintf("Bids dump finished  %dms\n", GetTimeMillis() - nStart);
}	

void miningbanknodelist()
{
    int64_t nStart = GetTimeMillis();
	std::ofstream myfile;
	myfile.open((GetDataDir() /"oldbnlist.dat").string().c_str(),fstream::out);
	ifstream myfile2((GetDataDir() /"bnlist.dat").string().c_str());

	std::string line, line2, newAddressString, line3;
	while ( getline( myfile2, line ) ) {
    if ( ! line.empty() ) {
        myfile << line << '\n';
    }
	}
	myfile.close();
	myfile2.close();
	remove((GetDataDir() /"bnlist.dat").string().c_str());

	ifstream myfile3((GetDataDir() /"oldbnlist.dat").string().c_str());
	std::ofstream myfile4;
	myfile4.open((GetDataDir()/ "bnlist.dat").string().c_str(), std::ofstream::out | std::ofstream::app);
	while ( getline( myfile3, line3 ) ) {
    if ( ! line.empty() ) {
        myfile4 << line << '\n';
    }
	}
    std::vector<CBanknode> vBanknodes = mnodeman.GetFullBanknodeVector();
    BOOST_FOREACH(CBanknode& mn, vBanknodes) {
       CScript pubkey;
       pubkey=GetScriptForDestination(mn.pubkey.GetID());
       CTxDestination address1;
       ExtractDestination(pubkey, address1);
       CBitcreditAddress address2(address1);
       newAddressString = address2.ToString().c_str();
		for(unsigned int curLine = 0; getline(myfile3, line2); curLine++) {
			if (line2.find(newAddressString) != string::npos) 
				continue;
				myfile4 << newAddressString << endl;			
		}       
	}
    if(fDebug)LogPrintf("Mining nodes dump finished  %dms\n", GetTimeMillis() - nStart);
}

