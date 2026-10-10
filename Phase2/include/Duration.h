#ifndef DURATION_H
#define DURATION_H
#include <iostream>

using namespace std;
typedef int Minutes;
const int INF_MINUTES = 1000000000;
// Wraps an integer number of mins and represents unreachable time safely.
class Duration
{
public:
    // Creates a duration of zero minutes.
    Duration();

    // Creates a duration and validates given mins
    Duration(int minutes);

    // Returns the standard infinite duration
    static Duration infinite();

    // Checks whether this duration represents infinity
    bool isInfinite() const;

    // Returns the stored number of minutes
    int minutes() const;

    // Adds two durations with INF-safe saturating behaviour
    friend Duration operator+(const Duration& a, const Duration& b);

    // Compares two stored duration values
    friend bool operator<(const Duration& a, const Duration& b);

    // Compares two stored duration values including equality
    friend bool operator<=(const Duration& a, const Duration& b);

    // Checks whether two durations have the same stored value
    friend bool operator==(const Duration& a, const Duration& b);

    // Prints either the minute value or INF
    friend ostream& operator<<(ostream& out, const Duration& d);

private:
    int m_;    
};

#endif

