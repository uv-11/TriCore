#include "EntityId.h"
#include "Exceptions.h"
#include <iomanip>
#include <sstream>

using namespace std;

// Creates the default empty id 
EntityId::EntityId()
{
  prefix_ = "";
  number_ = 0;
}

// Creates an id after validating its num part
EntityId::EntityId(const string& prefix, int number)
{
  if (number < 0)
  {
    stringstream ss;
    ss << "EntityId number cannot be negative: "<< prefix << number;
    throw StructureException(ss.str());
  }
prefix_ = prefix;
number_ = number;
}

// Parses id & validates its prefix & num part
EntityId EntityId::parse(const string& text,const string& expectedPrefix)
{
  if(expectedPrefix.empty())
  {
    throw FormatException("Expected prefix cannot be empty");
  }

  if(text.find(expectedPrefix) != 0)
  {
    throw FormatException( "Expected prefix '" + expectedPrefix + "' in ID: " + text);
  }

  string numPart = text.substr(expectedPrefix.length());

  if(numPart.empty())
  {
    throw FormatException("No numeric part in ID: " + text);
  }

  for(size_t i = 0; i < numPart.length(); i++)
  {
    if(numPart[i] < '0' || numPart[i] > '9')
    {throw FormatException("Non-numeric character in ID: " + text);
    }
  }

  int number = 0;
  stringstream ss(numPart);
  ss >> number;
  if(ss.fail())
  {
    throw FormatException("ID number is too large: " + text);
  }

return EntityId(expectedPrefix, number);
}

// Builds the formatted id with at least three num digit
string EntityId::toString() const
{
  stringstream ss;
  ss << prefix_;
  ss << setfill('0');
  ss << setw(3);
  ss << number_;
  return ss.str();
}
// Returns the num part of the id
int EntityId::number() const
{return number_;
}
// Orders ids by prefix first & nume val second
bool EntityId::operator<(const EntityId& other) const
{
  if (prefix_ != other.prefix_)
  {  return prefix_ < other.prefix_;
  }

  return number_ < other.number_;
}

// Checks both the prefix and num part fr equality
bool EntityId::operator==(const EntityId& other) const
{
  return prefix_ == other.prefix_
  && number_ == other.number_;
}

// Increments the id safely nd returns the updated object
EntityId& EntityId::operator++()
{
  if (number_ == 2147483647)
  {
    throw StructureException("EntityId number cannot exceed INT_MAX: " + toString());
  }
number_++;
return *this;
}

// Returns the old id nd then increments it safely.
EntityId EntityId::operator++(int)
{
  if (number_ == 2147483647)
   {
    throw StructureException("EntityId number cannot exceed INT_MAX: " + toString());
  }
  EntityId old = *this;
  number_++;
  return old;
}

// Prints the formated id
ostream& operator<<(ostream& out, const EntityId& id)
{
  out << id.toString();
  return out;
}

