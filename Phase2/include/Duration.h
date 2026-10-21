#ifndef DURATION_H
#define DURATION_H
#include <iostream>

using namespace std;
typedef int Minutes;
const int INF_MINUTES = 1000000000;
// wraps a number of minutes and represents unreachable time safely
class Duration
{
public:
    // creates a duration of zero minutes
    Duration();

    // creates a duration and validates the given minutes
    Duration(int minutes);

    // returns the standard infinite duration
    static Duration infinite();

    // checks whether this duration represents infinity
    bool isInfinite() const;

    // returns the stored number of minutes
    int minutes() const;

    // adds two durations with inf safe saturating behaviour
    friend Duration operator+(const Duration& a, const Duration& b);

    // compares two stored duration values
    friend bool operator<(const Duration& a, const Duration& b);

    // compares two stored duration values including equality
    friend bool operator<=(const Duration& a, const Duration& b);

    // checks whether two durations have the same stored value
    friend bool operator==(const Duration& a, const Duration& b);

    // prints either the minute value or inf
    friend ostream& operator<<(ostream& out, const Duration& d);

private:
    int m_;    
};

#endif
