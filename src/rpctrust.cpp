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
	TrustEngine t;
	string g = t.gettrust(search);
   
	o.push_back(Pair(params[0].get_str().c_str(),g));
	
    return o;
}
