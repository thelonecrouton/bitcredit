// Copyright (c) 2014 The Memorycoin developers
#include "voting.h"
#include "base58.h"
#include "utilstrencodings.h"
#include "chain.h"
#include "main.h"

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>

#include <sys/stat.h>
#include <stdio.h>

using namespace boost;
using namespace std;

