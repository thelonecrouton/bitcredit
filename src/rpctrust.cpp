#include "rpcserver.h"
#include "trust.h"

#include "json/json_spirit_value.h"
#include "sync.h"
#include "util.h"

using namespace json_spirit;

Value gettrust(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettrust  <ChainID>\n"
            "Returns an object containing information about a ChainID's trust and credit scores\n"
            "This should be used for debugging/confirmation purposes only; this is a resource intensive\n"
            "operation and may slow down wallet operation, especially on smaller computers.\n"
            "\nExamples:\n"
            + HelpExampleCli("gettrust", "61Xw5qzVvbvumfKMUDcRYpzq4ELzCPnP1N")
            + HelpExampleRpc("gettrust", "61Xw5qzVvbvumfKMUDcRYpzq4ELzCPnP1N")
        );
	
	string search =params[0].get_str().c_str();
	Object o;

	sqlite3 *rawdb;
	sqlite3_stmt *stmt;
	char *zErrMsg = 0;
	int rc;
	const char *sql;

    rc = sqlite3_open((GetDataDir() / "ratings/rawdata.db").string().c_str(), &rawdb);
    if( rc ){
     if (fDebug) LogPrintf("Can't open database: %s\n", sqlite3_errmsg(rawdb));
    }else{
      if (fDebug) LogPrintf("Opened database successfully\n");
    }
	
	sql="select * from RAWDATA where ADDRESS = ?";

	rc = sqlite3_prepare(rawdb,sql, strlen(sql), &stmt,  0 );
    sqlite3_bind_text(stmt, 1,search.data(), search.size(), 0);
    int64_t balance, outgoingtx, firstuse, incomingtx, totalinputs, totaloutputs;
	double avedailyincome;
	double trust=0;
	if (sqlite3_step(stmt) == SQLITE_ROW){

		balance = (sqlite3_column_int(stmt, 1))/COIN;
		firstuse = sqlite3_column_int(stmt, 2);
		incomingtx = sqlite3_column_int(stmt, 3);
		outgoingtx = sqlite3_column_int(stmt, 4);
		totalinputs = sqlite3_column_int(stmt, 5);
		totaloutputs = sqlite3_column_int(stmt, 6);

		int64_t totaltx=  incomingtx+outgoingtx;
		double globallife = (GetTime()- 1418504572)/24*3600;
		double lifetime = (GetTime() - firstuse)/24*3600;
		int64_t totalnettx = chainActive.Tip()->nChainTx;
		double txfreq = totaltx/lifetime;
		double nettxfreq = totalnettx / globallife;
		double spendfactor = balance/totaloutputs;
		double savefactor;

				if (totalinputs !=0)
					savefactor =balance/totalinputs;
				else
					savefactor = 0;

        double nettxpart = totaltx/ totalnettx;
        double aveinput = totalinputs/ incomingtx;
        double aveoutput = totaloutputs / outgoingtx;
        avedailyincome = totalinputs / lifetime;
        double avedailyexpenditure = totaloutputs / lifetime;

			{
				{
					if (lifetime > 360 )
							trust+= 20;
						else if (lifetime > 30 && lifetime < 360 )
							trust+= lifetime*0.055;
						else
							trust+= 0;
						}

					{
					if (totaltx > 10000){
						trust+= 10;
					}
					else if (totaltx>0 && totaltx< 10000){
						trust+= totaltx*0.001;
					}
					else
						trust+= 0;
				}

				{
					if(balance > 1000000){
						trust+= 25;
					}
					else if(balance > 0 && balance <= 1000000){
						trust+= balance/50000;
					}
					else
						trust+= 0;
				}

				{
					if (txfreq > 5)
						trust+=15;
					else if (txfreq> 0.01 && txfreq< 5)
						trust+= txfreq *3;
					else
						trust+= 0;
				}

				{
					if (savefactor > 0.1)
						trust+=20;
					else if (savefactor> 0.001 && savefactor< 0.1)
						trust+= savefactor *200;
					else
						trust+= 0;
				}

				{
					if (avedailyincome > 100)
						trust+=20;
					else if (avedailyincome > 1 && avedailyincome< 100)
						trust+= avedailyincome/5;
					else
						trust+= 0;
				}

				{
					int count = 0;
					char* minerquery = sqlite3_mprintf("select count(*) from BLOCKS where MINER = '%q'",search.c_str());
					rc = sqlite3_exec(rawdb, minerquery, callback, &count, &zErrMsg);
					double points = count/chainActive.Tip()->nHeight;

					if (points > 0.01 )
						trust+= 20;
					else if (points > 0.0001 && points < 0.01)
						trust+= points*190;
					else
						trust+= 0;
				}
			}
		}
	o.push_back(Pair("ChainID", search));
	o.push_back(Pair("Trust Rating", trust));
	o.push_back(Pair("Balance", balance));
	o.push_back(Pair("Daily Income", avedailyincome));
	//o.push_back(Pair("Credit Rating", credit));
	//o.push_back(Pair("Votes", votes));
	//o.push_back(Pair("Blacklisted", fBlacklisted));
	
    return o;
}
