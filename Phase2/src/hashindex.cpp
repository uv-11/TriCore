#include "hashindex.h"
#include "Exceptions.h"

using namespace std;

// Creates an empty table with this many buckets.
hashindex::hashindex(int buckets)
{
    if (buckets < 1)
    {
        throw StructureException("Hash table needs at least 1 bucket");
    }

    tablesize_ = buckets;
    count_ = 0;
    table_ = new node*[tablesize_];

    for (int pos = 0; pos < tablesize_; pos++)
    {
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
    int spot = hashof(key);
    node* current = table_[spot];

    while (current != NULL)
    {
        if (current->key == key)
        {
            return false;
        }

        current = current->next;
    }

    node* fresh = new node;
    fresh->key = key;
    fresh->value = value;
    fresh->next = table_[spot];
    table_[spot] = fresh;
    count_++;

    if (loadfactor() > 0.75)
    {
        rehash();
    }

    return true;
}

// Walks the list of the bucket of the key until the key is found or the list ends.
bool hashindex::get(const string& key, int& value) const
{
    node* current = table_[hashof(key)];

    while (current != NULL)
    {
        if (current->key == key)
        {
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
    return (double)count_ / tablesize_;
}

// Polynomial hash: code = code * 31 + character, reduced by the bucket count
// at every step.
int hashindex::hashof(const string& key) const
{
    long long code = 0;

    for (size_t pos = 0; pos < key.length(); pos++)
    {
        code = (code * 31 + (unsigned char)key[pos]) % tablesize_;
    }

    return (int)code;
}

// Builds a table twice as big and relinks every old node into it, so no node is copied.
void hashindex::rehash()
{
    int oldsize = tablesize_;
    node** old = table_;

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
            node* following = current->next;
            int spot = hashof(current->key);
            current->next = table_[spot];
            table_[spot] = current;
            current = following;
        }
    }

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
        node* tail = NULL;
        node* current = other.table_[pos];

        while (current != NULL)
        {
            node* fresh = new node;
            fresh->key = current->key;
            fresh->value = current->value;
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
            node* following = current->next;
            delete current;
            current = following;
        }
    }

    delete[] table_;
    table_ = NULL;
    tablesize_ = 0;
    count_ = 0;
}