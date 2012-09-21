#ifndef GLOBAL_FUNCS
#define GLOBAL_FUNCS

#include <string>
#include <vector>
#include <sstream>

template<typename ArgType> std::string any2str(const ArgType& from, int precision = -1) {
  std::stringstream to("");
  if (precision > -1) {
    to.precision(precision);
    to.setf(std::ios::fixed, std::ios::floatfield);
  }
  to << from;
  return to.str();
}

template<typename RetType>
RetType str2any(const std::string& from) {
  std::stringstream ssfrom(from);
  RetType to;
  ssfrom >> to;
  return to;
}

std::vector<std::string> split(const std::string& str, const std::string& seps = " \t\n", bool keepEmpty = false);
std::string ltrim(std::string str);
std::string rtrim(std::string str);
std::string trim(std::string str);


template<typename ArgType> int signum(const ArgType& x) {
  return (x > ArgType(0)) - (x < ArgType(0));
}


#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#endif
