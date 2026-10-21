#include "Exceptions.h"

using namespace std;

// stores the message in the base exception object
AppException::AppException(const string& message)
{
    message_ = message;
}

// keeps the destructor compatible with std exception
AppException::~AppException() throw()
{
}

// returns the stored message when what is called
const char* AppException::what() const throw()
{
    return message_.c_str();
}

// forwards a format error to the base class
FormatException::FormatException(const string& message)
    : AppException(message)
{
}

// forwards a data error to the base class
DataException::DataException(const string& message)
    : AppException(message)
{
}

// forwards a structure error to the base class
StructureException::StructureException(const string& message)
    : AppException(message)
{
}
