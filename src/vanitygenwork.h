#ifndef VANITYGENWORK_H
#define VANITYGENWORK_H

#include "pattern.h"
#include "vanity_util.h"
#include "util.h"
static vg_context_t *vcp = NULL;

extern double VanityGenHashrate;
extern double VanityGenKeysChecked;
extern int VanityGenNThreads;
extern int VanityGenMatchCase;

struct VanGenStruct{
    int id;
    std::string pattern;
    std::string privkey;
    std::string pubkey;
    std::string difficulty;

    //state 0 nothing
    //state 1 difficulty was recalculated
    //state 2 match was found
    //state 3 import was triggered (pattern can now be deleted from worklist)
    int state;
    int notification;
};

extern std::deque<VanGenStruct> VanityGenWorkList;

extern bool VanityGenRunning;

extern std::string VanityGenPassphrase;

extern bool AddressIsMine;

class VanityGenWork
{

public:

    char **pattern;
    int threads;
    int caseInsensitive;

    int setup();
    void doVanityGenWork();

    void stop_threads();

private:
    int start_threads(vg_context_t *vcp, int nthreads);
};

#endif // VANITYGENWORK_H
