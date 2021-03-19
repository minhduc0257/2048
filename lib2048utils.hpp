#include <stdio.h>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#define LOG(r) printf("%s", r);

#ifndef _2048UTILS
#define _2048UTILS

template<class T>
bool compareVector(std::vector<T> first, std::vector<T> second)
{
    if (first.size() != second.size()) return false;
    for (auto i = 0 ; i < first.size() ; i++)
        if (first[i] != second[i]) return false;
    return true;
}

inline std::vector<std::string> split (std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

inline std::string trim_left(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {return !std::isspace(ch);}));
    return s;
}

inline std::string trim_right(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {return !std::isspace(ch);}).base(), s.end());
    return s;
}

inline std::string trim(std::string s) {
    return trim_right(trim_left(s));
}


#endif