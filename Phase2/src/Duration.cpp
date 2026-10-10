#include "Duration.h"
#include "Exceptions.h"
#include <sstream>

using namespace std;

// Creates a zero-minute duration.
Duration::Duration()
{
  m_ = 0;
}

// create a valid duration or clamps large values to INF.
Duration::Duration(int minutes)
{
  if (minutes < 0)
  {
  stringstream ss;
  ss <<"Duration cannot be negative: " << minutes;
  throw StructureException(ss.str());
  }

  if(minutes>= INF_MINUTES)
  {
    m_ = INF_MINUTES;
  }
  else
  {
    m_ =minutes;
  }
}

// Returns the single standard value used for an unreachable duration.
Duration Duration::infinite()
{return Duration(INF_MINUTES);
}

// Checks whether the stored duration is exactly the INF value
bool Duration::isInfinite() const
{
  return m_ ==INF_MINUTES;
}

// Returns the stored duration in minutes
int Duration::minutes() const
{
  return m_;
}

// Adds two durations without allowing INF arithmetic to overflow
Duration operator+(const Duration& a,const Duration& b)
{
  if(a.isInfinite()||b.isInfinite())
  {
    return Duration::infinite();
  }
long long sum= (long long)a.m_ + b.m_;
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
// Checks equality using the stored duration values.
bool operator==(const Duration& a, const Duration& b)
{
  return a.m_== b.m_;
}
// Prints INF for unreachable durations and minutes otherwise.
ostream& operator<<(ostream& out, const Duration& d)
{
  if (d.isInfinite())
  {
    out <<"INF";
  }
  else
  { out <<d.m_;
  }
return out;
}



