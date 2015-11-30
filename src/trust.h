#ifndef TRUST_ENGINE_H
#define TRUST_ENGINE_H 

#include <stdio.h>
#include <algorithm>
#include <exception>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <boost/unordered_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>
#include <sqlite3.h>

using namespace std;

class TrustEngine
{
public:

	void createdb();

	void deletedb();

	void insertdb(string address);

	void selectfulldb();

	void selectrecorddb(string address);

	void showbncandidatesdb(string address);

	void updatedb();
	
	void buildrawdb();

	void sortclientdata(string clientAddress);
	
	string getidtrust(string search);
};

#endif
