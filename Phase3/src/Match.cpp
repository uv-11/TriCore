// Match.cpp: the one function of the match view.

#include "Match.h"

using namespace std;

// Creates an empty match view: no candidates, nothing offered yet.
matchview::matchview()
{
    effectiveremaining = 0;
    pendingoffers = 0;
}
