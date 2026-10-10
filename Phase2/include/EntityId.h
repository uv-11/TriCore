#ifndef ENTITYID_H
#define ENTITYID_H

#include <string>
#include <iostream>

using namespace std;

// Represents a project ID using a prefix and num part
class EntityId
{
public:
    // default empty ID.
    EntityId();

    // Creates an ID from its prefix and num part.
    EntityId(const string& prefix, int number);

    // Parses and validates an ID string against the expected prefix.
    static EntityId parse(const string& text,const string& expectedPrefix);

    // Id to text conversion with atleast three num digits.
    string toString() const;

    int number() const;

    // Orders ids by prefix and then num value
    bool operator<(const EntityId& other) const;

    // Checks whether both id parts are equal or not
    bool operator==(const EntityId& other) const;

    EntityId& operator++();
    EntityId operator++(int);

    // Prints the ID using its formatted string representation.
    friend ostream& operator<<(ostream& out, const EntityId& id);

private:
    string prefix_;   
    int number_;       
};

#endif

