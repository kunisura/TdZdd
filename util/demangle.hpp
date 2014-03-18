/*
 * Decode "mangled" name
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2011 Japan Science and Technology Agency
 * $Id: demangle.hpp 518 2014-03-18 05:29:23Z iwashita $
 */

#pragma once

#include <cstdlib>
#include <cxxabi.h>
#include <string>
#include <typeinfo>

namespace tdzdd {

inline std::string demangle(char const* name) {
    char* dName = abi::__cxa_demangle(name, 0, 0, 0);
    if (dName == 0) return name;

    std::string s;
    char* p = dName;

    for (char c = *p++; c; c = *p++) {
        s += c;
        if (!isalnum(c)) {
            while (std::isspace(*p)) {
                ++p;
            }
        }
    }

    free(dName);
    return s;
}

inline std::string demangleTypename(char const* name) {
    std::string s = demangle(name);
    size_t i = 0;
    size_t j = 0;

    while (j + 1 < s.size()) {
        if (std::isalnum(s[j])) {
            ++j;
        }
        else if (s[j] == ':' && s[j + 1] == ':') {
            s = s.replace(i, j + 2 - i, "");
            j = i;
        }
        else {
            i = ++j;
        }
    }

    return s;
}

template<typename T>
std::string typenameof() {
    return demangleTypename(typeid(T).name());
}

template<typename T>
std::string typenameof(T const& obj) {
    return demangleTypename(typeid(obj).name());
}

} // namespace tdzdd
