#include "Duration.h"
#include "Exceptions.h"
#include <sstream>

using namespace std;

// creates a zero minute duration
Duration::Duration()
{
    m_ = 0;
}

// creates a valid duration or clamps large values to inf
Duration::Duration(int minutes)
{
    if (minutes < 0)
    {
        stringstream ss;
        ss << "Duration cannot be negative: " << minutes;
        throw StructureException(ss.str());
    }

    if (minutes >= INF_MINUTES)
    {
        m_ = INF_MINUTES;
    }
    else
    {
        m_ = minutes;
    }
}

// returns the standard value used for an unreachable duration
Duration Duration::infinite()
{
    return Duration(INF_MINUTES);
}

// checks whether the stored duration is exactly inf
bool Duration::isInfinite() const
{
    return m_ == INF_MINUTES;
}

// returns the stored duration in minutes
int Duration::minutes() const
{
    return m_;
}

// adds two durations without allowing inf arithmetic to overflow
Duration operator+(const Duration& a, const Duration& b)
{
    if (a.isInfinite() || b.isInfinite())
    {
        return Duration::infinite();
    }

    long long sum = (long long)a.m_ + b.m_;

    return Duration((int)sum);
}

// compares the stored duration values
bool operator<(const Duration& a, const Duration& b)
{
    return a.m_ < b.m_;
}

// compares the stored duration values including equality
bool operator<=(const Duration& a, const Duration& b)
{
    return a.m_ <= b.m_;
}

// checks equality using the stored duration values
bool operator==(const Duration& a, const Duration& b)
{
    return a.m_ == b.m_;
}

// prints inf for unreachable durations and minutes otherwise
ostream& operator<<(ostream& out, const Duration& d)
{
    if (d.isInfinite())
    {
        out << "INF";
    }
    else
    {
        out << d.m_;
    }

    return out;
}
