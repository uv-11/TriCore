#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

using namespace std;

// base class for every exception the app can throw
class AppException : public exception
{
public:
  // stores the error message used by what
  AppException(const string& message);
  // allows safe destruction through a base class pointer
  virtual ~AppException() throw();
  // returns the stored message as a c style string
  virtual const char* what() const throw();

private:
    string message_;    // the stored error message
};

// invalid or badly formatted input text
class FormatException : public AppException
{
public:
    // passes the formatting error message to base class
    FormatException(const string& message);
};

// invalid or inconsistent data loaded from files
class DataException : public AppException
{
public:
  // passes the data error message to base class
  DataException(const string& message);
};

// incorrect use of an internal program structure
class StructureException : public AppException
{
public:
  // passes the structure error message to base class
  StructureException(const string& message);
};

#endif
