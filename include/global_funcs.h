#ifndef GLOBAL_FUNCS
#define GLOBAL_FUNCS

#include <math.h>

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <type_traits>

template<typename T>
struct EnumTraits {
  static const std::vector<std::string> data;
};


#define define_enum_strings(E) template<> const std::vector<std::string> EnumTraits<E>::data
#define STRING_ENUM true
#define NOT_STRING_ENUM false
template<bool>
class StringConverter {};

template<>
class StringConverter<NOT_STRING_ENUM> {
  StringConverter();
public:
  template<typename ArgType> static std::string any2str(const ArgType& from, int precision = -1) {
    std::stringstream to("");
    if (precision > -1) {
      to.precision(precision);
      to.setf(std::ios::fixed, std::ios::floatfield);
    }
    to << from;
    return to.str();
  }
  static std::string any2str(const std::string& from) { return from; } // null conversion (string to string)

  static std::string any2str(bool from) { //std::pair<std::string, std::string> boolstr = std::make_pair("true", "false")) {
    return from == true ? "true" : "false";
  }

  template<typename RetType> static RetType str2any(const std::string& from) {
    std::stringstream ssfrom(from);
    RetType to;
    ssfrom >> to;
    return to;
  }
};




template<>
class StringConverter<STRING_ENUM> {
  StringConverter();
public:
  template<typename T> static std::string any2str(const T& from, int) {
    return EnumTraits<T>::data[static_cast<typename std::underlying_type<T>::type>(from)];
  }
  template<typename T> static T str2any(const std::string& from) {
    static auto begin = std::begin(EnumTraits<T>::data);
    static auto end = std::end(EnumTraits<T>::data); // + EnumTraits<T>::data_size;
    return static_cast<T>(std::distance(begin, std::find(begin, end, from)));
  }
};



template<typename T> std::string any2str(const T& from, int precision = -1) {
  return StringConverter<std::is_enum<T>::value>::any2str(from, precision);
};



template<typename T> T str2any(const std::string& from) {
  return StringConverter<std::is_enum<T>::value>::template str2any<T>(from); // THIS SYNTAX IS HORRID +o(
};





std::vector<std::string> split(const std::string& str, const std::string& seps = " \t\n", bool keepEmpty = false);
template<class T> std::vector<T> split(const std::string& str, const std::string& seps = " \t\n", bool keepEmpty = false) {
  auto vs = split(str, seps, keepEmpty);
  std::vector<T> vt(vs.size());
  std::transform(vs.begin(), vs.end(), vt.begin(), [](std::string s) { return str2any<T>(s); });
  return vt;
}

template<class T, class I> std::string join(I begin, I end, const std::string& sep) {
  std::stringstream ss;
  std::copy(begin, end, std::ostream_iterator<T>(ss, sep.c_str()));
  return ss.str().substr(0, ss.str().length()-1);
}

template<class T> std::string join(const std::vector<T>& vec, const std::string& sep) { return join<T>(vec.begin(), vec.end(), sep); }


std::string ltrim(std::string str);
std::string rtrim(std::string str);
std::string trim(std::string str);
std::string lctrim(std::string str, const std::string& chars);
std::string rctrim(std::string str, const std::string& chars);
std::string ctrim(std::string str, const std::string& chars);


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

template<class I>
struct RangePair : std::pair<I, I> {
  RangePair(const std::pair<I, I>& p) : std::pair<I, I>(p) {}
  I begin() const { return this->first; }
  I end()   const { return this->second; }
};

template<class I> inline RangePair<I> pair2range(const std::pair<I, I>& p) { return RangePair<I>(p); }

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

inline double pt2radius(double pt, double magneticField) { return pt/(0.3*magneticField) * 1e3; }
inline double radius2pt(double radius, double magneticField) { return radius * 0.3 * magneticField * 1e-3; }
inline double eta2theta(double eta) { return 2*atan(exp(-eta)); }
inline double theta2eta(double theta) { return -log(tan(theta/2)); }


template<class I, class UnaryOperation> inline auto maxget(I begin, I end, UnaryOperation op) -> decltype(op(*begin)) {
  auto max = op(*begin++);
  for (auto it = begin; it != end; ++it) max = MAX(max, op(*it));
  return max;
}

template<class I, class UnaryOperation> inline auto minget(I begin, I end, UnaryOperation op) -> decltype(op(*begin)) {
  auto min = op(*begin++);
  for (auto it = begin; it != end; ++it) min = MIN(min, op(*it));
  return min;
}

#endif
