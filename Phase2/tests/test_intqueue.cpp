#include "intqueue.h"
#include "Exceptions.h"
#include <cstdlib>
#include <iostream>
#include <queue>
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

// Main test runner. WHY: Executes all intqueue unit tests.
int main()
{
    cout << "--- test_intqueue ---\n";

    // QU1: first in, first out, and the same answers as std::queue on seeded random work
    intqueue queue1;
    queue1.enqueue(1);
    queue1.enqueue(2);
    queue1.enqueue(3);

    check(queue1.dequeue() == 1, "QU1: first dequeue returns 1");
    check(queue1.dequeue() == 2, "QU1: second dequeue returns 2");
    check(queue1.dequeue() == 3, "QU1: third dequeue returns 3");
    check(queue1.empty(), "QU1: empty after dequeuing everything");

    srand(12345);
    intqueue mine;
    queue<int> oracle;
    bool same = true;

    for (int counter = 0; counter < 2000; counter++)
    {
        if (oracle.empty() || rand() % 3 != 0)
        {
            int value = rand() % 1000;
            mine.enqueue(value);
            oracle.push(value);
        }
        else
        {
            int got = mine.dequeue();
            int expected = oracle.front();
            oracle.pop();

            if (got != expected)
            {
                same = false;
            }
        }

        if (mine.size() != (int)oracle.size())
        {
            same = false;
        }
    }

    check(same, "QU1: 2000 random enqueue/dequeue steps agree with std::queue");

    // QU2: wrap-around. Capacity 4 never holds more than 3 values, so it never grows,
    // but the front moves around the array many times
    intqueue wrap(4);
    bool wrapok = true;
    int next = 0;
    int expected = 0;

    wrap.enqueue(next);
    next++;
    wrap.enqueue(next);
    next++;

    for (int counter = 0; counter < 50; counter++)
    {
        wrap.enqueue(next);
        next++;

        if (wrap.dequeue() != expected)
        {
            wrapok = false;
        }

        expected++;
    }

    check(wrapok, "QU2: order stays correct while the front wraps around a capacity 4 array");
    check(wrap.size() == 2, "QU2: size is 2 after the wrap-around cycles");

    // QU3: growth when full loses nothing, even when the values were wrapped
    intqueue small(4);
    small.enqueue(100);
    small.enqueue(101);
    small.enqueue(102);
    small.dequeue();
    small.dequeue();

    for (int counter = 0; counter < 50; counter++)
    {
        small.enqueue(counter);
    }

    check(small.size() == 51, "QU3: size is 51 after growing from capacity 4");
    check(small.dequeue() == 102, "QU3: the wrapped value 102 is still at the front");

    bool ordered = true;

    for (int counter = 0; counter < 50; counter++)
    {
        if (small.dequeue() != counter)
        {
            ordered = false;
        }
    }

    check(ordered, "QU3: all values after growth come out in order");

    // QU4: dequeue on an empty queue throws
    intqueue emptyqueue;
    bool threw = false;

    try
    {
        emptyqueue.dequeue();
    }
    catch (const StructureException&)
    {
        threw = true;
    }

    check(threw, "QU4: dequeue on empty throws StructureException");

    // Copy constructor and operator= are deep copies, also when the queue is wrapped
    intqueue original(4);
    original.enqueue(1);
    original.enqueue(2);
    original.enqueue(3);
    original.dequeue();
    original.enqueue(4);
    original.enqueue(5);

    intqueue copy(original);
    copy.dequeue();
    copy.enqueue(6);

    check(original.size() == 4, "QU4: original size unchanged after changing the copy");
    check(original.dequeue() == 2, "QU4: original front unchanged after changing the copy");

    intqueue assigned;
    assigned = original;
    assigned.enqueue(7);

    check(original.size() == 3, "QU4: original unchanged after changing an operator= copy");

    assigned = assigned;
    check(assigned.size() == 4, "QU4: self-assignment keeps the values");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}
