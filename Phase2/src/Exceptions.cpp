#include "Exceptions.h"

using namespace std;

// Stores the message in the base exception object.
AppException::AppException(const string& message)
{
    message_ = message;
}

// Keeps the base exception destructor compatible with std::exception
AppException::~AppException() throw(){}


// Returns the stored mess when what() is called
const char* AppException::what() const throw()
{
    return message_.c_str();
}

// Forwards a format error to the common exception implement
FormatException::FormatException(const string& message): AppException(message){}


// Forwards a data error to the common exception implement
DataException::DataException(const string& message): AppException(message){}


// Forwards a struc error to the common exception implement
StructureException::StructureException(const string& message): AppException(message){}


