#include "intqueue.h"
#include "Exceptions.h"

using namespace std;

// Creates an empty queue with room for start values.
intqueue::intqueue(int start)
{
    if (start < 1)
    {
        throw StructureException("Queue capacity must be at least 1");
    }

    items_ = new int[start];
    front_ = 0;
    count_ = 0;
    capacity_ = start;
}

// Copies every stored value into a new array.
intqueue::intqueue(const intqueue& other)
{
    items_ = new int[other.capacity_];
    front_ = 0;
    count_ = other.count_;
    capacity_ = other.capacity_;

    for (int pos = 0; pos < count_; pos++)
    {
        items_[pos] = other.items_[(other.front_ + pos) % other.capacity_];
    }
}

// Copies the other queue after the new array is ready, then frees the old one.
intqueue& intqueue::operator=(const intqueue& other)
{
    if (this != &other)
    {
        int* fresh = new int[other.capacity_];

        for (int pos = 0; pos < other.count_; pos++)
        {
            fresh[pos] = other.items_[(other.front_ + pos) % other.capacity_];
        }

        delete[] items_;
        items_ = fresh;
        front_ = 0;
        count_ = other.count_;
        capacity_ = other.capacity_;
    }

    return *this;
}

// Frees the array.
intqueue::~intqueue()
{
    delete[] items_;
    items_ = NULL;
}

// Puts a value at the back and doubles the array when full.
void intqueue::enqueue(int value)
{
    if (count_ == capacity_)
    {
        grow();
    }

    int back = (front_ + count_) % capacity_;
    items_[back] = value;
    count_++;
}

// Alias for enqueue.
void intqueue::push(int value)
{
    enqueue(value);
}

// Removes the front value and returns it. Throws StructureException when empty.
int intqueue::dequeue()
{
    if (count_ == 0)
    {
        throw StructureException("dequeue on an empty intqueue");
    }

    int value = items_[front_];
    front_ = (front_ + 1) % capacity_;
    count_--;
    return value;
}

// Alias for dequeue.
int intqueue::pop()
{
    return dequeue();
}

// Checks whether the queue has no values.
bool intqueue::empty() const
{
    return count_ == 0;
}

// Returns how many values are stored.
int intqueue::size() const
{
    return count_;
}

// Allocates an array twice as big and unwraps the values into it.
void intqueue::grow()
{
    int biggerCapacity = capacity_ * 2;
    int* bigger = new int[biggerCapacity];

    for (int pos = 0; pos < count_; pos++)
    {
        bigger[pos] = items_[(front_ + pos) % capacity_];
    }

    delete[] items_;
    items_ = bigger;
    front_ = 0;
    capacity_ = biggerCapacity;
}
