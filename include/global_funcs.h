#ifndef GLOBAL_FUNCS
#define GLOBAL_FUNCS

#include <math.h>

#include <string>
#include <vector>
#include <sstream>

template<typename T>
struct EnumReflex {
  static const char* data[];
  static bool is_string_enum = sizeof(data);
};


template<typename T> string enum2str(const T& e) {
  return EnumReflex<T>::data[static_cast<std::underlying_type<T>>(e)];
};

template<typename T> T str2enum(const string& s) {
  static auto begin = std::begin(EnumStrings<T>::data);
  static auto end = std::end(EnumStrings<T>::data);
  return static_cast<T>(std::distance(begin, std::find(begin, end, s));
};



template<typename ArgType> std::string any2str(const ArgType& from, int precision = -1) {
  if (EnumReflex<ArgType>::is_string_enum) {
    return enum2str(from);
  } else {
    std::stringstream to("");
    if (precision > -1) {
      to.precision(precision);
      to.setf(std::ios::fixed, std::ios::floatfield);
    }
    to << from;
    return to.str();
  }
}

template<typename RetType> RetType str2any(const std::string& from) {
  if (EnumReflex<RetType>::is_string_enum) {
    return str2enum<RetType>(s);
  } else {
    std::stringstream ssfrom(from);
    RetType to;
    ssfrom >> to;
    return to;
  }
}

std::vector<std::string> split(const std::string& str, const std::string& seps = " \t\n", bool keepEmpty = false);
std::string ltrim(std::string str);
std::string rtrim(std::string str);
std::string trim(std::string str);


template<typename ArgType> int signum(const ArgType& x) {
  return (x > ArgType(0)) - (x < ArgType(0));
}

template<int Precision, typename ArgRetType> ArgRetType roundprec(const ArgRetType& x) {
  static const float p = pow(10., Precision);
  return floor(x * p + 0.5) / p;
}


template<int Magnification, typename ArgType> int mapint(const ArgType& x) {
  static const float p = pow(10., Magnification);
  return floor(x * p + 0.5);
}


#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#endif
