#include "EntityId.h"
#include "Exceptions.h"
#include <iostream>
#include <string>
#include <sstream>

// Defines INT_MAX for testing.
#include <climits>

using namespace std;

int failCount = 0;
int gapFail = 0;

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

// Checks a known gap condition and prints PASS or GAP FAIL. WHY: Tracks expected logic holes.
void gapCheck(bool condition, const string& name)
{
    if (condition)
    {
        cout << "PASS " << name << "\n";
    }
    else
    {
        cout << "GAP FAIL " << name << "\n";
        gapFail++;
    }
}

// Main test runner. WHY: Executes all EntityId unit tests.
int main()
{
    cout << "--- test_entityid ---\n";
    
    // ID1: test constructor and prefix/number
    EntityId id1("R", 7);
    check(id1.number() == 7, "ID1: constructor");
    
    EntityId defId;
    check(defId.number() == 0, "ID1: default constructor number");
    check(defId.toString() == "000", "ID1: default constructor toString");
    
    // ID2: test parse (success and throw)
    EntityId parsed = EntityId::parse("DON012", "DON");
    check(parsed.number() == 12, "ID2: parse success");
    
    bool threw = false;
    
    try
    {
        EntityId::parse("X12", "DON");
    }
    catch (const FormatException&)
    {
        threw = true;
    }
    check(threw, "ID2: parse throws FormatException on mismatch");
    
    // ID3: test operator<
    EntityId id2("R", 999);
    EntityId id3("R", 1000);
    check(id2 < id3, "ID3: numeric comparison (R999 < R1000)");
    
    EntityId idPrefixA("A", 100);
    EntityId idPrefixB("B", 10);
    check(idPrefixA < idPrefixB, "ID3: < across different prefixes");
    check(!(idPrefixB < idPrefixA), "ID3: < across different prefixes (reverse)");
    
    check(id2 == EntityId("R", 999), "ID3: operator== equal");
    check(!(id2 == id3), "ID3: operator== not equal");
    
    // ID4: test operator++ (pre/post)
    EntityId id4("T", 10);
    EntityId prev = id4++;
    check(prev.number() == 10 && id4.number() == 11, "ID4: post-increment");
    check(prev == EntityId("T", 10), "ID4: post-increment return value");
    
    EntityId& ref = ++id4;
    check(ref.number() == 12 && id4.number() == 12, "ID4: pre-increment");
    check(ref == EntityId("T", 12), "ID4: pre-increment return value");
    
    // ID5: test toString formatting
    check(id1.toString() == "R007", "ID5: zero-padded toString");
    check(id3.toString() == "R1000", "ID5: toString without padding if large");
    
    stringstream ssId;
    ssId << id1;
    check(ssId.str() == "R007", "ID5: << output");
    
    // ID6-ID7
    bool th = false;
    
    try
    {
        EntityId::parse("", "R");
    }
    catch (const FormatException&)
    {
        th = true;
    }
    check(th, "ID6: empty string throws");

    th = false;
    try
    {
        EntityId::parse("R", "R");
    }
    catch (const FormatException&)
    {
        th = true;
    }
    check(th, "ID6: 'R' throws");

    th = false;
    try
    {
        EntityId::parse("R12x", "R");
    }
    catch (const FormatException&)
    {
        th = true;
    }
    check(th, "ID6: 'R12x' throws");

    th = false;
    try
    {
        EntityId::parse("R-5", "R");
    }
    catch (const FormatException&)
    {
        th = true;
    }
    check(th, "ID6: 'R-5' throws");

    th = false;
    try
    {
        EntityId::parse("R007", "DON");
    }
    catch (const FormatException&)
    {
        th = true;
    }
    check(th, "ID6: wrong prefix throws");

    EntityId r007 = EntityId::parse("R007", "R");
    check(r007.number() == 7, "ID7: 'R007' parses to number 7");

    // KNOWN GAPS
    bool gap1 = false;
    
    try
    {
        EntityId::parse("R99999999999", "R");
    }
    catch (const FormatException&)
    {
        gap1 = true;
    }
    gapCheck(gap1, "parse(\"R99999999999\", \"R\") must throw FormatException");
    
    bool gap2 = false;
    
    try
    {
        EntityId("R", -5);
    }
    catch (const StructureException&)
    {
        gap2 = true;
    }
    gapCheck(gap2, "EntityId(\"R\", -5) must throw StructureException");
    
    bool gap3 = false;
    
    try
    {
        EntityId maxId("R", INT_MAX);
        ++maxId;
    }
    catch (const StructureException&)
    {
        gap3 = true;
    }
    gapCheck(gap3, "++ at INT_MAX must throw StructureException");
    
    bool gap4 = false;
    
    try
    {
        EntityId::parse("123", "");
    }
    catch (const FormatException&)
    {
        gap4 = true;
    }
    gapCheck(gap4, "parse(\"123\", \"\") must throw FormatException");

    cout << "Total FAIL: " << failCount << "\n";
    cout << "Known-gap FAIL: " << gapFail << "\n";

    if (failCount > 0 || gapFail > 0)
    {
        return 1;
    }
    
    return 0;
}
