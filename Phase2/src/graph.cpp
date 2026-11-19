#include "intstack.h"
#include "Exceptions.h"

using namespace std;

// Creates an empty stack with room for start values.
intstack::intstack(int start)
{
    if (start < 1)
    {
        throw StructureException("Stack capacity must be at least 1");
    }

    items_ = new int[start];
    count_ = 0;
    capacity_ = start;
}

// Copies every stored value into a new array.
intstack::intstack(const intstack& other)
{
    items_ = new int[other.capacity_];
    count_ = other.count_;
    capacity_ = other.capacity_;

    for (int pos = 0; pos < count_; pos++)
    {
        items_[pos] = other.items_[pos];
    }
}

// Copies the other stack after the new array is ready, then frees the old one.
intstack& intstack::operator=(const intstack& other)
{
    if (this != &other)
    {
        int* fresh = new int[other.capacity_];

        for (int pos = 0; pos < other.count_; pos++)
        {
            fresh[pos] = other.items_[pos];
        }

        delete[] items_;
        items_ = fresh;
        count_ = other.count_;
        capacity_ = other.capacity_;
    }

    return *this;
}

// Frees the array.
intstack::~intstack()
{
    delete[] items_;
    items_ = NULL;
}

// Stores the value after the last one.
void intstack::push(int value)
{
    if (count_ == capacity_)
    {
        grow();
    }

    items_[count_] = value;
    count_++;
}

// Removes the last stored value and returns it.
int intstack::pop()
{
    if (count_ == 0)
    {
        throw StructureException("pop on an empty intstack");
    }

    count_--;
    return items_[count_];
}

// Checks whether the stack has no values.
bool intstack::empty() const
{
    return count_ == 0;
}

// Returns how many values are stored.
int intstack::size() const
{
    return count_;
}

// Allocates an array twice as big and copies the values into it.
void intstack::grow()
{
    int* bigger = new int[capacity_ * 2];

    for (int pos = 0; pos < count_; pos++)
    {
        bigger[pos] = items_[pos];
    }

    delete[] items_;
    items_ = bigger;
    capacity_ = capacity_ * 2;
}
