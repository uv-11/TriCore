// hashindex.cpp: a hash table from a string key to an int position (Spec A20).

// Idea: the hash of a key gives a bucket number, so a lookup goes straight to one
// bucket instead of searching all keys. Keys that share a bucket (a collision) are
// kept in a linked list in that bucket (separate chaining). When there are more than
// 0.75 keys per bucket the table doubles, so the lists stay short and put and get
// cost O(1) on average (more memory for more speed). The table stores positions in a
// vector, never pointers, so it stays valid when the vector grows.

#include "hashindex.h"
#include "Exceptions.h"

using namespace std;

// Creates an empty table with this many buckets.
hashindex::hashindex(int buckets)
{
    // buckets is the number of lists (slots) at first. At least 1 is needed.
    if (buckets < 1)
    {
        throw StructureException("Hash table needs at least 1 bucket");
    }

    // tablesize_ is the number of buckets.
    tablesize_ = buckets;
    // count_ is how many keys are stored. A new table is empty.
    count_ = 0;
    // table_ is an array of pointers: table_[i] points to the first node of bucket i, or
    // is NULL when the bucket is empty.
    table_ = new node*[tablesize_];

    // pos is the bucket number.
    for (int pos = 0; pos < tablesize_; pos++)
    {
        // Every bucket starts empty (NULL means no node).
        table_[pos] = NULL;
    }
}

// Copies every node of the other table.
hashindex::hashindex(const hashindex& other)
{
    copyfrom(other);
}

// Frees the old nodes and then copies the other table.
hashindex& hashindex::operator=(const hashindex& other)
{
    if (this != &other)
    {
        clear();
        copyfrom(other);
    }

    return *this;
}

// Frees every node and the bucket array.
hashindex::~hashindex()
{
    clear();
}

// Adds a new node at the front of its bucket unless the key is already there.
bool hashindex::put(const string& key, int value)
{
    // spot is the bucket where this key belongs.
    int spot = hashof(key);
    // current walks along the list of that bucket, node by node.
    node* current = table_[spot];

    while (current != NULL)
    {
        // The key is already stored, so nothing is added and put returns false.
        if (current->key == key)
        {
            return false;
        }

        // Move on to the next node of the list.
        current = current->next;
    }

    // fresh is the new node. It goes at the front of its list. That costs O(1) and the
    // order inside one bucket does not matter.
    node* fresh = new node;
    fresh->key = key;
    fresh->value = value;
    fresh->next = table_[spot];
    table_[spot] = fresh;
    // One more key is stored.
    count_++;

    // Keeping the load factor at 0.75 or below keeps the lists short.
    if (loadfactor() > 0.75)
    {
        rehash();
    }

    return true;
}

// Walks the list of the bucket of the key until the key is found or the list ends.
bool hashindex::get(const string& key, int& value) const
{
    // Go straight to the bucket of the key (this is why a hash table is fast), then
    // search only that short list.
    node* current = table_[hashof(key)];

    while (current != NULL)
    {
        if (current->key == key)
        {
            // value is an output parameter (int&): the caller receives the stored
            // position here.
            value = current->value;
            return true;
        }

        current = current->next;
    }

    return false;
}

// Returns how many keys are stored.
int hashindex::size() const
{
    return count_;
}

// Stored keys divided by buckets.
double hashindex::loadfactor() const
{
    // (double) keeps the decimals. Dividing two ints would round down, so 3 / 4 would be
    // 0.
    return (double)count_ / tablesize_;
}

// Polynomial hash: code = code * 31 + character, reduced by the bucket count
// at every step.
int hashindex::hashof(const string& key) const
{
    // code is the hash that is built up. long long and the % at every step stop an
    // overflow, and give the same bucket as reducing once at the end.
    long long code = 0;

    // pos is the index of the letter that is added to the hash.
    for (size_t pos = 0; pos < key.length(); pos++)
    {
        // 31 is the base of the polynomial hash. (unsigned char) turns the letter into a
        // number from 0 to 255.
        code = (code * 31 + (unsigned char)key[pos]) % tablesize_;
    }

    return (int)code;
}

// Builds a table twice as big and relinks every old node into it, so no node is copied.
void hashindex::rehash()
{
    // A bucket number depends on the table size, so every node must move to the bucket it
    // has in the bigger table. The nodes are relinked, not copied.
    int oldsize = tablesize_;
    // old keeps the old array of list heads while the new one is built.
    node** old = table_;

    // The table doubles in size.
    tablesize_ = oldsize * 2;
    table_ = new node*[tablesize_];

    for (int pos = 0; pos < tablesize_; pos++)
    {
        table_[pos] = NULL;
    }

    for (int pos = 0; pos < oldsize; pos++)
    {
        node* current = old[pos];

        while (current != NULL)
        {
            // following is saved first, because current->next is about to change.
            node* following = current->next;
            // The bucket number is computed again, because the table size has changed.
            int spot = hashof(current->key);
            // Link the node in front of the list of its new bucket.
            current->next = table_[spot];
            table_[spot] = current;
            current = following;
        }
    }

    // Only the old array of list heads is freed. The nodes now belong to the new table.
    delete[] old;
}

// Allocates the same number of buckets and copies each list in the same order.
void hashindex::copyfrom(const hashindex& other)
{
    tablesize_ = other.tablesize_;
    count_ = other.count_;
    table_ = new node*[tablesize_];

    for (int pos = 0; pos < tablesize_; pos++)
    {
        table_[pos] = NULL;
        // tail is the last copied node of this bucket, so the copies keep the same order.
        node* tail = NULL;
        // current walks along the list of the other table.
        node* current = other.table_[pos];

        while (current != NULL)
        {
            node* fresh = new node;
            fresh->key = current->key;
            fresh->value = current->value;
            // The new node is the last one for now.
            fresh->next = NULL;

            if (tail == NULL)
            {
                table_[pos] = fresh;
            }
            else
            {
                tail->next = fresh;
            }

            tail = fresh;
            current = current->next;
        }
    }
}

// Deletes the nodes of every bucket and then the bucket array.
void hashindex::clear()
{
    for (int pos = 0; pos < tablesize_; pos++)
    {
        node* current = table_[pos];

        while (current != NULL)
        {
            // following is saved first, because current is gone after the delete.
            node* following = current->next;
            // Free one node (it was made with new).
            delete current;
            current = following;
        }
    }

    // Free the array of list heads.
    delete[] table_;
    table_ = NULL;
    tablesize_ = 0;
    count_ = 0;
}