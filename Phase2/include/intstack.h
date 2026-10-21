#ifndef INTSTACK_H
#define INTSTACK_H

// Stack of ints in a dynamic array. Used to print a route from donor to recipient.
class intstack
{
public:
    // Creates an empty stack with room for start values.
    intstack(int start = 16);

    // Makes a deep copy so both stacks own separate arrays.
    intstack(const intstack& other);

    // Replaces this stack with a deep copy of another one.
    intstack& operator=(const intstack& other);

    // Frees the array.
    ~intstack();

    // Puts a value on top and doubles the array when it is full.
    void push(int value);

    // Removes and returns the top value. Throws StructureException when empty.
    int pop();

    // Checks whether the stack has no values.
    bool empty() const;

    // Returns how many values are stored.
    int size() const;

private:
    // Array that holds the values
    int* items_;
    // How many values are stored
    int count_;
    // Size of the array
    int capacity_;

    // Doubles the capacity and copies the values.
    void grow();
};

#endif
