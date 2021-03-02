#include "SFML/Graphics.hpp"
#include <stdio.h>
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

#endif