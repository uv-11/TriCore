#include "priorityqueue.h"
#include "Exceptions.h"

using namespace std;

// Orders two items by key1 and then by key2.
static bool before(const heapitem& first, const heapitem& second)
{
    if (first.key1 != second.key1)
    {
        return first.key1 < second.key1;
    }

    return first.key2 < second.key2;
}

// Creates an empty heap with room for start items.
priorityqueue::priorityqueue(int start)
{
    if (start < 1)
    {
        throw StructureException("Heap capacity must be at least 1");
    }

    items_ = new heapitem[start];
    size_ = 0;
    capacity_ = start;
}

// Copies every stored item into a new array.
priorityqueue::priorityqueue(const priorityqueue& other)
{
    items_ = new heapitem[other.capacity_];
    size_ = other.size_;
    capacity_ = other.capacity_;

    for (int pos = 0; pos < size_; pos++)
    {
        items_[pos] = other.items_[pos];
    }
}

// Copies the other heap after the new array is ready, then frees the old one.
priorityqueue& priorityqueue::operator=(const priorityqueue& other)
{
    if (this != &other)
    {
        heapitem* fresh = new heapitem[other.capacity_];

        for (int pos = 0; pos < other.size_; pos++)
        {
            fresh[pos] = other.items_[pos];
        }

        delete[] items_;
        items_ = fresh;
        size_ = other.size_;
        capacity_ = other.capacity_;
    }

    return *this;
}

// Frees the array.
priorityqueue::~priorityqueue()
{
    delete[] items_;
    items_ = NULL;
}

// Puts the item at the end and lets it rise to its place.
void priorityqueue::push(const heapitem& item)
{
    if (size_ == capacity_)
    {
        grow();
    }

    items_[size_] = item;
    siftup(size_);
    size_++;
}

// Takes the root, moves the last item to the root and lets it sink.
heapitem priorityqueue::popmin()
{
    if (size_ == 0)
    {
        throw StructureException("popmin on an empty priorityqueue");
    }

    heapitem top = items_[0];
    size_--;
    items_[0] = items_[size_];

    if (size_ > 0)
    {
        siftdown(0);
    }

    return top;
}

// Returns the root, which is always the smallest item.
const heapitem& priorityqueue::peekmin() const
{
    if (size_ == 0)
    {
        throw StructureException("peekmin on an empty priorityqueue");
    }

    return items_[0];
}

// Checks whether the heap has no items.
bool priorityqueue::empty() const
{
    return size_ == 0;
}

// Returns how many items are stored.
int priorityqueue::size() const
{
    return size_;
}

// Compares every item with its parent to confirm the heap order.
bool priorityqueue::checkheap() const
{
    for (int pos = 1; pos < size_; pos++)
    {
        int parent = (pos - 1) / 2;

        if (before(items_[pos], items_[parent]))
        {
            return false;
        }
    }

    return true;
}

// Swaps the item with its parent while it is smaller than the parent.
void priorityqueue::siftup(int pos)
{
    while (pos > 0)
    {
        int parent = (pos - 1) / 2;

        if (before(items_[pos], items_[parent]))
        {
            heapitem temp = items_[pos];
            items_[pos] = items_[parent];
            items_[parent] = temp;
            pos = parent;
        }
        else
        {
            return;
        }
    }
}

// Swaps the item with its smaller child while that child is smaller than the item.
void priorityqueue::siftdown(int pos)
{
    while (2 * pos + 1 < size_)
    {
        int smaller = 2 * pos + 1;

        if (smaller + 1 < size_ && before(items_[smaller + 1], items_[smaller]))
        {
            smaller = smaller + 1;
        }

        if (before(items_[smaller], items_[pos]))
        {
            heapitem temp = items_[pos];
            items_[pos] = items_[smaller];
            items_[smaller] = temp;
            pos = smaller;
        }
        else
        {
            return;
        }
    }
}

// Allocates an array twice as big and copies the items into it.
void priorityqueue::grow()
{
    heapitem* bigger = new heapitem[capacity_ * 2];

    for (int pos = 0; pos < size_; pos++)
    {
        bigger[pos] = items_[pos];
    }

    delete[] items_;
    items_ = bigger;
    capacity_ = capacity_ * 2;
}