#include "Exceptions.h"
#include "Models.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Verifies that string-to-enum and enum-to-string operations perfectly invert each other.
void testEnums()
{
    cout << "--- Testing Enums ---\n";
    
    if (strToFood(foodToStr(COOKED)) == COOKED)
    {
        cout << "PASS: foodType round trip (COOKED)\n";
    }
    else
    {
        cout << "FAIL: foodType round trip (COOKED)\n";
    }
    
    if (strToStatus(statusToStr(PARTIAL)) == PARTIAL)
    {
        cout << "PASS: donationStatus round trip (PARTIAL)\n";
    }
    else
    {
        cout << "FAIL: donationStatus round trip (PARTIAL)\n";
    }
    
    try
    {
        strToFood("INVALID");
        cout << "FAIL: foodTypeFromString did not throw on bad text\n";
    }
    catch (const FormatException& e)
    {
        cout << "PASS: foodTypeFromString threw FormatException on bad text\n";
    }

    // F5 test: cast out-of-range int to enum and expect StructureException
    try
    {
        FoodType badEnum = (FoodType)999;
        foodToStr(badEnum);
        cout << "FAIL: foodTypeToString did not throw on invalid enum\n";
    }
    catch (const StructureException& e)
    {
        cout << "PASS: foodTypeToString threw StructureException on invalid enum\n";
    }
}

// Verifies that the Donor class correctly forms its CSV string according to Spec 5.2.
void testDonor()
{
    cout << "--- Testing Donor ---\n";
    
    EntityId dId("DN", 123);
    Donor d(dId, "Big Catering", "CATERER", 5);
    
    string expected = "DN123,Big Catering,CATERER,5";

    if (d.toCsvRow() == expected)
    {
        cout << "PASS: Donor toCsvRow matches Spec 5.2\n";
    }
    else
    {
        cout << "FAIL: Donor toCsvRow. Expected: " << expected << " Got: " << d.toCsvRow() << "\n";
    }
}

// Verifies that the Recipient class correctly forms its CSV string and safely manages capacity logic.
void testRecipient()
{
    cout << "--- Testing Recipient ---\n";
    
    EntityId rId("R", 42);
    vector<FoodType> accepted;
    accepted.push_back(COOKED);
    accepted.push_back(PACKAGED);
    
    Recipient r(rId, "Local Shelter", "SHELTER", 100, accepted, true, false, 7);
    
    string expected = "R042,Local Shelter,SHELTER,100,COOKED;PACKAGED,1,0,7";

    if (r.toCsvRow() == expected)
    {
        cout << "PASS: Recipient toCsvRow matches Spec 5.2\n";
    }
    else
    {
        cout << "FAIL: Recipient toCsvRow. Expected: " << expected << " Got: " << r.toCsvRow() << "\n";
    }
    
    if (r.accepts(COOKED) && r.accepts(PACKAGED) && !r.accepts(PRODUCE))
    {
        cout << "PASS: Recipient accepts() for single and multiple types\n";
    }
    else
    {
        cout << "FAIL: Recipient accepts() logic is flawed\n";
    }
    
    r.reduceCapacity(40);

    if (r.availableCapacity() == 60)
    {
        cout << "PASS: Recipient reduceCapacity (normal)\n";
    }
    else
    {
        cout << "FAIL: Recipient reduceCapacity (normal)\n";
    }
    
    r.reduceCapacity(60);

    if (r.availableCapacity() == 0)
    {
        cout << "PASS: Recipient reduceCapacity (exact-to-zero)\n";
    }
    else
    {
        cout << "FAIL: Recipient reduceCapacity (exact-to-zero)\n";
    }
    
    try
    {
        r.reduceCapacity(10);
        cout << "FAIL: Recipient reduceCapacity did not throw beyond capacity\n";
    }
    catch (const StructureException& e)
    {
        cout << "PASS: Recipient reduceCapacity threw beyond capacity\n";
    }
}

// Verifies that the Donation class correctly forms its CSV string and correctly handles allocations and status changes.
void testDonation()
{
    cout << "--- Testing Donation ---\n";
    
    EntityId id("DON", 1);
    EntityId dId("DN", 10);
    Donation d(id, dId, PRODUCE, 50, 50, 120, PENDING);
    
    string expected = "DON001,DN010,PRODUCE,50,50,120,PENDING";

    if (d.toCsvRow() == expected)
    {
        cout << "PASS: Donation toCsvRow matches Spec 5.2\n";
    }
    else
    {
        cout << "FAIL: Donation toCsvRow. Expected: " << expected << " Got: " << d.toCsvRow() << "\n";
    }
    
    // F1 test: allocate 0 throws, state unchanged
    try
    {
        d.allocateQuantity(0);
        cout << "FAIL: Donation allocated 0 without throwing\n";
    }
    catch (const StructureException& e)
    {
        if (d.status() == PENDING && d.remainingQuantity() == 50)
        {
            cout << "PASS: Donation allocation of 0 rejected, state unchanged\n";
        }
        else
        {
            cout << "FAIL: Donation allocation of 0 changed state\n";
        }
    }
    
    // F2 test: close from PENDING
    Donation dPending(id, dId, PRODUCE, 50, 50, 120, PENDING);
    dPending.close();

    if (dPending.status() == CLOSED && dPending.remainingQuantity() == 50)
    {
        cout << "PASS: close() works from PENDING, remaining unchanged\n";
    }
    else
    {
        cout << "FAIL: close() from PENDING broke state\n";
    }

    d.allocateQuantity(20);

    if (d.status() == PARTIAL && d.remainingQuantity() == 30)
    {
        cout << "PASS: Donation status changed to PARTIAL\n";
    }
    else
    {
        cout << "FAIL: Donation status changed to PARTIAL\n";
    }
    
    // F2 test: close from PARTIAL
    Donation dPartial(id, dId, PRODUCE, 50, 30, 120, PARTIAL);
    dPartial.close();

    if (dPartial.status() == CLOSED && dPartial.remainingQuantity() == 30)
    {
        cout << "PASS: close() works from PARTIAL, remaining unchanged\n";
    }
    else
    {
        cout << "FAIL: close() from PARTIAL broke state\n";
    }

    d.allocateQuantity(30);

    if (d.status() == COMPLETED && d.remainingQuantity() == 0)
    {
        cout << "PASS: Donation status changed to COMPLETED\n";
    }
    else
    {
        cout << "FAIL: Donation status changed to COMPLETED\n";
    }
    
    try
    {
        d.allocateQuantity(10);
        cout << "FAIL: Donation allocated from COMPLETED without throwing\n";
    }
    catch (const StructureException& e)
    {
        cout << "PASS: Donation allocation from COMPLETED rejected\n";
    }

    // F2 test: throw from COMPLETED
    try
    {
        d.close();
        cout << "FAIL: close() from COMPLETED did not throw\n";
    }
    catch (const StructureException& e)
    {
        if (d.status() == COMPLETED && d.remainingQuantity() == 0)
        {
            cout << "PASS: close() from COMPLETED rejected, state unchanged\n";
        }
        else
        {
            cout << "FAIL: close() from COMPLETED altered state\n";
        }
    }

    // F2 test: throw from CLOSED
    try
    {
        dPending.close();
        cout << "FAIL: close() from CLOSED did not throw\n";
    }
    catch (const StructureException& e)
    {
        if (dPending.status() == CLOSED && dPending.remainingQuantity() == 50)
        {
            cout << "PASS: close() from CLOSED rejected, state unchanged\n";
        }
        else
        {
            cout << "FAIL: close() from CLOSED altered state\n";
        }
    }
}

// Verifies that the Transaction class correctly forms its CSV string according to Spec 5.2.
void testTransaction()
{
    cout << "--- Testing Transaction ---\n";
    
    EntityId id("T", 100);
    EntityId donId("DON", 5);
    EntityId rId("R", 8);
    Transaction t(id, donId, rId, 25, 15);
    
    string expected = "T100,DON005,R008,25,15";

    if (t.toCsvRow() == expected)
    {
        cout << "PASS: Transaction toCsvRow matches Spec 5.2\n";
    }
    else
    {
        cout << "FAIL: Transaction toCsvRow. Expected: " << expected << " Got: " << t.toCsvRow() << "\n";
    }
}

// Verifies that polymorphism works correctly: calling toCsvRow() through a base pointer invokes the derived class implementation.
void testBinding()
{
    cout << "--- Testing Late Binding and Deletion ---\n";
    
    EntityId dId("DN", 99);
    CsvRecord* record1 = new Donor(dId, "Test Donor", "CATERER", 2);
    
    if (record1->toCsvRow() == "DN099,Test Donor,CATERER,2")
    {
        cout << "PASS: Donor through CsvRecord pointer gave right row (late binding)\n";
    }
    else
    {
        cout << "FAIL: Donor late binding\n";
    }
    
    delete record1; // Safe because CsvRecord has a virtual destructor.
    cout << "PASS: Delete through base pointer didn't crash\n";
}

// Main test runner.
int main()
{
    cout << "Starting Models Tests...\n\n";
    
    testEnums();
    cout << "\n";
    
    testDonor();
    cout << "\n";
    
    testRecipient();
    cout << "\n";
    
    testDonation();
    cout << "\n";
    
    testTransaction();
    cout << "\n";
    
    testBinding();
    cout << "\n";
    
    cout << "All tests finished.\n";
    
    return 0;
}
