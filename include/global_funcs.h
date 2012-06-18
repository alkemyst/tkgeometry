#ifndef GLOBAL_FUNCS
#define GLOBAL_FUNCS

template<typename ArgType> std::string any2str(const ArgType& from, int precision = -1) {
    std::stringstream to("");
    if (precision > -1) {
        to.precision(precision);
        to.setf(ios::fixed, ios::floatfield);
    }
    to << from;
    return to.str();
}

template<typename RetType>
RetType str2any(const string& from) {
    std::stringstream ssfrom(from);
    RetType to;
    ssfrom >> to;
    return to;
}

#endif
