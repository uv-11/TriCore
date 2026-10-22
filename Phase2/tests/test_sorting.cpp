#include "sorting.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

int failcount = 0;

// A value with a sort key and the position where it started. Used for the stability test.
struct entry
{
    int key;
    int order;
};

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

// Orders entries by key only, so entries with equal keys count as equal.
bool lessbykey(const entry& first, const entry& second)
{
    return first.key < second.key;
}

// Orders ints from large to small.
bool greaterint(const int& first, const int& second)
{
    return first > second;
}

// Fills the array for one input kind: 0 random, 1 sorted, 2 reversed, 3 many duplicates.
void fillarray(int* items, int count, int kind)
{
    for (int pos = 0; pos < count; pos++)
    {
        if (kind == 0)
        {
            items[pos] = rand() % 100000;
        }
        else if (kind == 1)
        {
            items[pos] = pos;
        }
        else if (kind == 2)
        {
            items[pos] = count - pos;
        }
        else
        {
            items[pos] = rand() % 5;
        }
    }
}

// Main test runner. WHY: Executes all sorting unit tests.
int main()
{
    cout << "--- test_sorting ---\n";

    // SO1: mergesort gives the same array as std::sort for every kind and size
    srand(12345);
    int sizes[6] = {0, 1, 2, 9, 100, 1000};
    string kinds[4] = {"random", "sorted", "reversed", "duplicates"};
    bool allsame = true;

    for (int kind = 0; kind < 4; kind++)
    {
        bool kindsame = true;

        for (int which = 0; which < 6; which++)
        {
            int count = sizes[which];
            int* mine = new int[count + 1];
            int* oracle = new int[count + 1];

            fillarray(mine, count, kind);

            for (int pos = 0; pos < count; pos++)
            {
                oracle[pos] = mine[pos];
            }

            mergesort(mine, count);
            sort(oracle, oracle + count);

            for (int pos = 0; pos < count; pos++)
            {
                if (mine[pos] != oracle[pos])
                {
                    kindsame = false;
                }
            }

            delete[] mine;
            delete[] oracle;
        }

        check(kindsame, "SO1: " + kinds[kind] + " arrays of n = 0, 1, 2, 9, 100, 1000 match std::sort");

        if (!kindsame)
        {
            allsame = false;
        }
    }

    check(allsame, "SO1: operator< version matches std::sort on every input kind");

    // SO1: the comparison function version, here largest first
    int down[8] = {4, 9, 1, 7, 7, 3, 0, 5};
    int downoracle[8] = {4, 9, 1, 7, 7, 3, 0, 5};

    mergesort(down, 8, greaterint);
    sort(downoracle, downoracle + 8, greaterint);

    bool downsame = true;

    for (int pos = 0; pos < 8; pos++)
    {
        if (down[pos] != downoracle[pos])
        {
            downsame = false;
        }
    }

    check(downsame, "SO1: comparison function version matches std::sort with the same function");

    // SO2: equal keys keep their original order
    entry entries[200];

    for (int pos = 0; pos < 200; pos++)
    {
        entries[pos].key = rand() % 6;
        entries[pos].order = pos;
    }

    mergesort(entries, 200, lessbykey);

    bool sorted = true;
    bool stable = true;

    for (int pos = 1; pos < 200; pos++)
    {
        if (entries[pos].key < entries[pos - 1].key)
        {
            sorted = false;
        }

        if (entries[pos].key == entries[pos - 1].key &&
            entries[pos].order < entries[pos - 1].order)
        {
            stable = false;
        }
    }

    check(sorted, "SO2: entries are sorted by key");
    check(stable, "SO2: entries with equal keys keep their original order");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}