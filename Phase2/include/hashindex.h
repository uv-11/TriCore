#ifndef HASHINDEX_H
#define HASHINDEX_H

#include <string>

using namespace std;

// Hash table that maps a string key to an int position. Keys that land in the
// same bucket are chained in a linked list. Used for every ID check when the
// CSV files are loaded.
class hashindex
{
public:
    // Creates an empty table with this many buckets. It grows by itself when the load
    // passes 0.75.
    // DESIGN CHOICE (not in Spec): the argument is a bucket count, not an item count.
    hashindex(int buckets = 16);

    // Makes a deep copy so both tables own separate nodes.
    hashindex(const hashindex& other);

    // Replaces this table with a deep copy of another one.
    hashindex& operator=(const hashindex& other);

    // Frees every node and the bucket array.
    ~hashindex();

    // Stores key and value. Returns false and keeps the old value if the key
    // already exists.
    bool put(const string& key, int value);

    // Looks up key. Returns true and sets value if found, otherwise returns false.
    bool get(const string& key, int& value) const;

    // Returns how many keys are stored.
    int size() const;

    // Returns stored keys divided by buckets. The table grows when this passes 0.75.
    double loadfactor() const;

private:
    // One entry in the linked list of a bucket
    struct node
    {
        string key;
        int value;
        node* next;
    };

    // Array with the first node of every bucket
    node** table_;
    // Number of buckets
    int tablesize_;
    // How many keys are stored
    int count_;

    // Turns a key into a bucket number with a polynomial hash (base 31).
    int hashof(const string& key) const;

    // Doubles the number of buckets and moves every node to its new bucket.
    void rehash();

    // Copies every node of another table into this one.
    void copyfrom(const hashindex& other);

    // Frees every node and the bucket array.
    void clear();
};

#endif