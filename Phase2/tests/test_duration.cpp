#include "Duration.h"
#include "Exceptions.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

int failCount = 0;

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
        failCount++;
    }
}

// Main test runner. WHY: Executes all Duration unit tests.
int main()
{
    cout << "--- test_duration ---\n";
    
    // DU1: test construction and default
    Duration d0;
    check(d0.minutes() == 0, "DU1: default constructor");
    
    Duration d1(15);
    check(d1.minutes() == 15, "DU1: parameterized constructor");
    
    check(Duration(2000000000) == Duration::infinite(), "DU1: Duration(2000000000) == infinite()");
    check(Duration::infinite().isInfinite(), "DU1: isInfinite() is true for infinite");
    check(!Duration(5).isInfinite(), "DU1: isInfinite() is false for 5");
    check(Duration(7).minutes() == 7, "DU1: minutes() getter");

    // DU2: test saturating operator+
    Duration inf = Duration::infinite();
    Duration sum1 = d1 + Duration(10);
    check(sum1.minutes() == 25, "DU2: plain addition");
    
    Duration sum2 = d1 + inf;
    check(sum2.isInfinite(), "DU2: saturating addition (x + INF)");
    
    check((Duration::infinite() + Duration(5)).isInfinite(), "DU2: infinite() + 5 is infinite");

    // DU3: test relational operators
    check(d1 < sum1, "DU3: operator<");
    check(d1 <= d1, "DU3: operator<=");
    check(sum1 == Duration(25), "DU3: operator==");
    
    check(!(Duration(5) < Duration(3)), "DU3: 5 < 3 is false");
    check(Duration(3) < Duration::infinite(), "DU3: 3 < infinite is true");

    // DU4: test stream output
    cout << "DU4 visual check: d1=" << d1 << " inf=" << inf << "\n";
    check(true, "DU4: stream output compiles");

    stringstream ss1;
    ss1 << Duration(12);
    check(ss1.str() == "12", "DU4: output of Duration(12) is 12");
    
    stringstream ss2;
    ss2 << Duration::infinite();
    check(ss2.str() == "INF", "DU4: output of infinite() is INF");

    // DU5: Edge cases for addition
    check((Duration(999999999) + Duration(999999999)) == inf, "DU5: 999999999 + 999999999 == INF");
    check((inf + inf) == inf, "DU5: INF + INF == INF");
    check((Duration(INF_MINUTES - 1) + Duration(5)) == inf, "DU5: (INF - 1) + 5 == INF");
    check((Duration(0) + Duration(0)) == Duration(0), "DU5: 0 + 0 == 0");

    // DU6: Exception throws
    bool threwNegative = false;
    
    try
    {
        Duration(-5);
    }
    catch (const StructureException&)
    {
        threwNegative = true;
    }
    check(threwNegative, "DU6: Duration(-5) throws StructureException");
    
    bool threwZero = false;
    
    try
    {
        Duration(0);
    }
    catch (const StructureException&)
    {
        threwZero = true;
    }
    check(!threwZero, "DU6: Duration(0) is fine");

    cout << "Total FAIL: " << failCount << "\n";

    if (failCount > 0)
    {
        return 1;
    }
    
    return 0;
}
