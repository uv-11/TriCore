#include "priorityqueue.h"
#include "Exceptions.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

using namespace std;

int failcount = 0;

// STL min-heap used only as a test oracle. Order is key1, key2, payload.
typedef pair<int, pair<int, int> > oracleitem;
typedef priority_queue<oracleitem, vector<oracleitem>, greater<oracleitem> > oraclequeue;

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

// Builds a heap item from three numbers.
heapitem makeitem(int key1, int key2, int payload)
{
    heapitem item;
    item.key1 = key1;
    item.key2 = key2;
    item.payload = payload;
    return item;
}

// Builds the matching oracle entry.
oracleitem makeoracle(const heapitem& item)
{
    return oracleitem(item.key1, pair<int, int>(item.key2, item.payload));
}

// Main test runner. WHY: Executes all priorityqueue unit tests.
int main()
{
    cout << "--- test_priorityqueue ---\n";

    // H1: 1000 seeded random items must pop in the same order as the STL heap
    srand(12345);
    priorityqueue heap;
    oraclequeue oracle;

    for (int pos = 0; pos < 1000; pos++)
    {
        heapitem item = makeitem(rand() % 500, pos, rand() % 100000);
        heap.push(item);
        oracle.push(makeoracle(item));
    }

    bool same = true;

    while (!oracle.empty())
    {
        heapitem got = heap.popmin();
        oracleitem expected = oracle.top();
        oracle.pop();

        if (got.key1 != expected.first || got.key2 != expected.second.first ||
            got.payload != expected.second.second)
        {
            same = false;
        }
    }

    check(same, "H1: 1000 random items pop in the same order as std::priority_queue");
    check(heap.empty(), "H1: heap is empty after popping everything");

    // H2: equal key1, key2 decides
    priorityqueue ties;
    ties.push(makeitem(5, 30, 1));
    ties.push(makeitem(5, 10, 2));
    ties.push(makeitem(5, 20, 3));
    ties.push(makeitem(2, 99, 4));

    check(ties.popmin().payload == 4, "H2: smaller key1 comes first");
    check(ties.popmin().payload == 2, "H2: equal key1, key2 10 comes next");
    check(ties.popmin().payload == 3, "H2: equal key1, key2 20 comes next");
    check(ties.popmin().payload == 1, "H2: equal key1, key2 30 comes last");

    // H3: pushing past the start capacity loses nothing
    priorityqueue small(2);

    for (int pos = 0; pos < 100; pos++)
    {
        small.push(makeitem(100 - pos, pos, pos));
    }

    check(small.size() == 100, "H3: size is 100 after pushing past capacity 2");

    bool ordered = true;

    for (int pos = 1; pos <= 100; pos++)
    {
        if (small.popmin().key1 != pos)
        {
            ordered = false;
        }
    }

    check(ordered, "H3: all 100 items come out in order");

    // H4: popmin and peekmin on an empty heap throw
    priorityqueue emptyheap;
    bool threwpop = false;
    bool threwpeek = false;

    try
    {
        emptyheap.popmin();
    }
    catch (const StructureException&)
    {
        threwpop = true;
    }

    try
    {
        emptyheap.peekmin();
    }
    catch (const StructureException&)
    {
        threwpeek = true;
    }

    check(threwpop, "H4: popmin on empty throws StructureException");
    check(threwpeek, "H4: peekmin on empty throws StructureException");

    // H5: copy constructor and operator= are deep copies
    priorityqueue original;
    original.push(makeitem(3, 1, 30));
    original.push(makeitem(1, 2, 10));
    original.push(makeitem(2, 3, 20));

    priorityqueue copy(original);
    copy.popmin();
    copy.push(makeitem(0, 4, 99));

    check(original.size() == 3, "H5: original size unchanged after changing the copy");
    check(original.peekmin().payload == 10, "H5: original root unchanged after changing the copy");
    check(copy.peekmin().payload == 99, "H5: copy has its own new root");

    priorityqueue assigned;
    assigned = original;
    assigned.popmin();

    check(original.size() == 3, "H5: original size unchanged after operator= copy is changed");

    assigned = assigned;
    check(assigned.size() == 2, "H5: self-assignment keeps the items");

    // H6: heap property holds after every push and pop
    priorityqueue checked;
    bool alwaysheap = true;

    for (int pos = 0; pos < 500; pos++)
    {
        checked.push(makeitem(rand() % 50, pos, pos));

        if (!checked.checkheap())
        {
            alwaysheap = false;
        }

        if (pos % 3 == 2)
        {
            checked.popmin();

            if (!checked.checkheap())
            {
                alwaysheap = false;
            }
        }
    }

    while (!checked.empty())
    {
        checked.popmin();

        if (!checked.checkheap())
        {
            alwaysheap = false;
        }
    }

    check(alwaysheap, "H6: heap property holds after every push and pop");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}
