#include "Exceptions.h"
#include <iostream>
#include <string>

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

// Main test runner. WHY: Executes all Exceptions unit tests.
int main()
{
    cout << "--- test_exceptions ---\n";
    
    // EX2: test exception polymorphism and what()
    bool caughtBase = false;
    
    try
    {
        throw DataException("Missing file");
    }
    catch (const AppException& e)
    {
        caughtBase = true;
        string msg(e.what());
        check(msg == "Missing file", "EX2: what() returns correct message for DataException");
    }
    check(caughtBase, "EX2: derived exception caught by base reference");

    caughtBase = false;
    try
    {
        throw FormatException("Bad format");
    }
    catch (const AppException& e)
    {
        caughtBase = true;
        string msg(e.what());
        check(msg == "Bad format", "EX2: what() returns correct message for FormatException");
    }
    check(caughtBase, "EX2: FormatException caught by base reference");

    caughtBase = false;
    try
    {
        throw StructureException("Bad structure");
    }
    catch (const AppException& e)
    {
        caughtBase = true;
        string msg(e.what());
        check(msg == "Bad structure", "EX2: what() returns correct message for StructureException");
    }
    check(caughtBase, "EX2: StructureException caught by base reference");

    cout << "Total FAIL: " << failCount << "\n";

    if (failCount > 0)
    {
        return 1;
    }
    
    return 0;
}
