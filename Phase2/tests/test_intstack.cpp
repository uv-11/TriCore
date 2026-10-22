#include "intstack.h"
#include "Exceptions.h"
#include <cstdlib>
#include <iostream>
#include <stack>
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

// Main test runner. WHY: Executes all intstack unit tests.
int main()
{
    cout << "--- test_intstack ---\n";

    // ST1: last in, first out, and the same answers as std::stack on seeded random work
    intstack stack1;
    stack1.push(1);
    stack1.push(2);
    stack1.push(3);

    check(stack1.pop() == 3, "ST1: first pop returns 3");
    check(stack1.pop() == 2, "ST1: second pop returns 2");
    check(stack1.pop() == 1, "ST1: third pop returns 1");
    check(stack1.empty(), "ST1: empty after popping everything");

    srand(12345);
    intstack mine;
    stack<int> oracle;
    bool same = true;

    for (int counter = 0; counter < 2000; counter++)
    {
        if (oracle.empty() || rand() % 3 != 0)
        {
            int value = rand() % 1000;
            mine.push(value);
            oracle.push(value);
        }
        else
        {
            int got = mine.pop();
            int expected = oracle.top();
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

    check(same, "ST1: 2000 random push/pop steps agree with std::stack");

    // ST2: popping an empty stack throws
    intstack emptystack;
    bool threw = false;

    try
    {
        emptystack.pop();
    }
    catch (const StructureException&)
    {
        threw = true;
    }

    check(threw, "ST2: pop on empty throws StructureException");

    // ST3: growth past the start capacity loses nothing
    intstack small(2);

    for (int counter = 0; counter < 100; counter++)
    {
        small.push(counter);
    }

    check(small.size() == 100, "ST3: size is 100 after pushing past capacity 2");

    bool ordered = true;

    for (int counter = 99; counter >= 0; counter--)
    {
        if (small.pop() != counter)
        {
            ordered = false;
        }
    }

    check(ordered, "ST3: all 100 values come out in reverse order");

    // Copy constructor and operator= are deep copies
    intstack original;
    original.push(7);
    original.push(8);

    intstack copy(original);
    copy.pop();
    copy.push(9);
    copy.push(10);

    check(original.size() == 2, "ST3: original size unchanged after changing the copy");
    check(original.pop() == 8, "ST3: original top unchanged after changing the copy");

    intstack assigned;
    assigned = original;
    assigned.push(5);

    check(original.size() == 1, "ST3: original unchanged after changing an operator= copy");

    assigned = assigned;
    check(assigned.size() == 2, "ST3: self-assignment keeps the values");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}
