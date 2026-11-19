#ifndef INTQUEUE_H
#define INTQUEUE_H

// Queue of ints on a circular dynamic array.
class intqueue
{
public:
    // Creates an empty queue with room for start values.
    intqueue(int start = 16);

    // Makes a deep copy so both queues own separate arrays.
    intqueue(const intqueue& other);

    // Replaces this queue with a deep copy of another one.
    intqueue& operator=(const intqueue& other);

    // Frees the array.
    ~intqueue();

    // Puts a value at the back and doubles the array when full.
    void enqueue(int value);
    void push(int value);

    // Removes and returns the front value. Throws StructureException when empty.
    int dequeue();
    int pop();

    // Checks whether the queue has no values.
    bool empty() const;

    // Returns how many values are stored.
    int size() const;

private:
    int* items_;
    int front_;
    int count_;
    int capacity_;

    // Doubles the capacity and unwraps the values into the new array.
    void grow();
};

#endif
