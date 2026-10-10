#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

using namespace std;

// Its a base exception for all application exceptions which mah rise during execution of program
class AppException : public exception
{
public:
  // stores the error message used by what()
  AppException(const string& message);
  // Allows safe destruction through a base class pointer as required by std::exception
  virtual ~AppException() throw();
  // Returns the stored error message as a C-style string because the base class std::exception requires it
  virtual const char* what() const throw();

private:
    string message_;    // Stores the complete app error mess
};

// Represents invalid or badly formatted input text
class FormatException : public AppException
{
public:
    // passes the formatting error mess to base class 
    FormatException(const string& message);
};

// Represents invalid / inconsistent data loaded from files
class DataException : public AppException
{
public:
  DataException(const string& message);
};

// Represents the incorrect use of an internal program structure
class StructureException : public AppException
{
public:
  StructureException(const string& message);
};

#endif


