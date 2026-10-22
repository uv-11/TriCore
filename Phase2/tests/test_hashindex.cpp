#include "hashindex.h"
#include "Exceptions.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

using namespace std;

int failcount = 0;

// Checks a condition and prints PASS or FAIL. WHY: Helper for tests.
void check(bool condition, const string& name)
{
    if (condition)
    {
        cout << "PASS " << name << "\n";
    }
    else
    {
        cout << "FAIL " << name << "\n";
        failcount++;
    }
}

// Builds a key like K42 from a number.
string makekey(int number)
{
    stringstream text;
    text << "K" << number;
    return text.str();
}

// Main test runner. WHY: Executes all hashindex unit tests.
int main()
{
    cout << "--- test_hashindex ---\n";

    // HT1: put, get and a missing key
    hashindex index;
    int found = -1;

    check(index.put("R001", 0), "HT1: put of a new key returns true");
    check(index.put("R002", 1), "HT1: put of a second key returns true");
    check(index.get("R002", found) && found == 1, "HT1: get finds R002 with value 1");
    check(index.get("R001", found) && found == 0, "HT1: get finds R001 with value 0");

    found = -1;
    check(!index.get("R999", found), "HT1: get of a missing key returns false");
    check(found == -1, "HT1: a failed get leaves the value untouched");
    check(index.size() == 2, "HT1: size is 2");

    // HT2: Aa, BB, AaAa, AaBB, BBAa and BBBB have the same base 31 hash, so they always
    // share one bucket. A table with one bucket forces collisions for every key as well.
    hashindex crowded(1);
    string same[6] = {"Aa", "BB", "AaAa", "AaBB", "BBAa", "BBBB"};
    bool allput = true;

    for (int pos = 0; pos < 6; pos++)
    {
        if (!crowded.put(same[pos], pos * 10))
        {
            allput = false;
        }
    }

    check(allput, "HT2: all colliding keys can be stored");

    bool allfound = true;

    for (int pos = 0; pos < 6; pos++)
    {
        if (!crowded.get(same[pos], found) || found != pos * 10)
        {
            allfound = false;
        }
    }

    check(allfound, "HT2: all colliding keys are still found with the right value");

    // HT3: crossing the load factor 0.75 rehashes and loses nothing
    hashindex growing(4);
    growing.put("K0", 0);
    growing.put("K1", 1);
    growing.put("K2", 2);
    check(growing.loadfactor() == 0.75, "HT3: 3 keys in 4 buckets is exactly 0.75, no rehash");

    growing.put("K3", 3);
    check(growing.loadfactor() == 0.5, "HT3: the fourth key passes 0.75, table doubles to 0.5");

    bool belowlimit = true;

    for (int pos = 4; pos < 300; pos++)
    {
        growing.put(makekey(pos), pos);

        if (growing.loadfactor() > 0.75)
        {
            belowlimit = false;
        }
    }

    check(belowlimit, "HT3: load factor never stays above 0.75");

    bool keptall = true;

    for (int pos = 0; pos < 300; pos++)
    {
        if (!growing.get(makekey(pos), found) || found != pos)
        {
            keptall = false;
        }
    }

    check(keptall && growing.size() == 300, "HT3: all 300 keys found after many rehashes");

    // HT4: a duplicate put returns false and keeps the old value
    hashindex dup;
    dup.put("DON001", 5);

    check(!dup.put("DON001", 9), "HT4: duplicate put returns false");
    check(dup.get("DON001", found) && found == 5, "HT4: old value 5 is kept");
    check(dup.size() == 1, "HT4: size is still 1");

    // HT5: copy constructor and operator= are deep copies
    hashindex original(2);
    original.put("A", 1);
    original.put("B", 2);
    original.put("C", 3);

    hashindex copy(original);
    copy.put("D", 4);

    check(original.size() == 3, "HT5: original size unchanged after adding to the copy");
    check(!original.get("D", found), "HT5: original does not see the key added to the copy");
    check(copy.get("A", found) && found == 1, "HT5: copy has the original keys");

    hashindex assigned;
    assigned.put("X", 7);
    assigned = original;
    assigned.put("E", 5);

    check(assigned.get("B", found) && found == 2, "HT5: operator= copies the keys");
    check(!assigned.get("X", found), "HT5: operator= drops the old keys");
    check(!original.get("E", found), "HT5: original does not see the key added to the assigned copy");

    assigned = assigned;
    check(assigned.size() == 4, "HT5: self-assignment keeps the keys");

    // HT6: 1000 seeded random puts agree with std::map
    srand(12345);
    hashindex mine;
    map<string, int> oracle;
    bool sameput = true;

    for (int pos = 0; pos < 1000; pos++)
    {
        string key = makekey(rand() % 700);
        bool added = mine.put(key, pos);
        bool oracleadded = oracle.insert(pair<string, int>(key, pos)).second;

        if (added != oracleadded)
        {
            sameput = false;
        }
    }

    check(sameput, "HT6: put returns the same true or false as std::map insert");

    bool samelookup = true;

    for (int pos = 0; pos < 1000; pos++)
    {
        string key = makekey(pos);
        int got = -1;
        bool hasmine = mine.get(key, got);
        map<string, int>::iterator spot = oracle.find(key);
        bool hasoracle = (spot != oracle.end());

        if (hasmine != hasoracle)
        {
            samelookup = false;
        }
        else if (hasmine && got != spot->second)
        {
            samelookup = false;
        }
    }

    check(samelookup, "HT6: get agrees with std::map for keys K0 to K999");
    check(mine.size() == (int)oracle.size(), "HT6: size agrees with std::map");

    // A bucket count below 1 is a structure mistake
    bool threw = false;

    try
    {
        hashindex bad(0);
    }
    catch (const StructureException&)
    {
        threw = true;
    }

    check(threw, "HT6: zero buckets throws StructureException");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}