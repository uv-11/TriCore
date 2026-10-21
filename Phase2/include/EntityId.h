#ifndef ENTITYID_H
#define ENTITYID_H

#include <string>
#include <iostream>

using namespace std;

// represents a project id using a prefix and a number part
class EntityId
{
public:
    // creates a default empty id
    EntityId();

    // creates an id from a prefix and a number
    EntityId(const string& prefix, int number);

    // parses and validates an id string against the expected prefix
    static EntityId parse(const string& text,const string& expectedPrefix);

    // converts the id to text with at least three digits
    string toString() const;

    // returns the number part of the id
    int number() const;

    // orders ids by prefix and then by number
    bool operator<(const EntityId& other) const;

    // checks whether both id parts are equal
    bool operator==(const EntityId& other) const;

    // increments the id and returns the updated object
    EntityId& operator++();

    // increments the id and returns the old value
    EntityId operator++(int);

    // prints the id using its formatted string
    friend ostream& operator<<(ostream& out, const EntityId& id);

private:
    // the id prefix
    string prefix_;   
    // the id number
    int number_;       
};

#endif
