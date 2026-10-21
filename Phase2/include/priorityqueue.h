#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

// One entry of the heap. Entries are ordered by key1 first and key2 second.
struct heapitem
{
    int key1;
    int key2;
    int payload;
};

// Binary min-heap stored in a dynamic array. Used for the urgency queue and for Dijkstra.
class priorityqueue
{
public:
    // Creates an empty heap with room for start items.
    priorityqueue(int start = 16);

    // Makes a deep copy so both heaps own separate arrays.
    priorityqueue(const priorityqueue& other);

    // Replaces this heap with a deep copy of another one.
    priorityqueue& operator=(const priorityqueue& other);

    // Frees the array.
    ~priorityqueue();

    // Adds an item and doubles the array when it is full.
    void push(const heapitem& item);

    // Removes and returns the smallest item. Throws StructureException when
    // empty.
    heapitem popmin();

    // Returns the smallest item without removing it. Throws StructureException
    // when empty.
    const heapitem& peekmin() const;

    // Checks whether the heap has no items.
    bool empty() const;

    // Returns how many items are stored.
    int size() const;

    // Checks that no child is smaller than its parent. Used by the tests.
    bool checkheap() const;

private:
    // Array that holds the heap items
    heapitem* items_;
    // How many items are stored
    int size_;
    // Size of the array
    int capacity_;

    // Moves the item at pos up until its parent is not larger.
    void siftup(int pos);

    // Moves the item at pos down until both children are not smaller.
    void siftdown(int pos);

    // Doubles the capacity and copies the items.
    void grow();
};

#endif
