#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#ifndef _2048UTILS
#define _2048UTILS

inline auto split(const std::string_view &s, std::string_view delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::vector<std::string_view> res;

    while ((pos_end = s.find_first_of(delimiter, pos_start)) !=
           std::string_view::npos) {
        auto token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

constexpr auto trim(std::string_view s) {
    auto front = s.find_first_not_of(' ');
    auto back = s.find_last_not_of(' ');
    return s.substr(front, back + 1 - front);
}

template <class T> class CyclingValues {
  private:
    std::vector<T> values;
    std::size_t currentIndex = 0;

  public:
    explicit CyclingValues(std::initializer_list<T> values,
                           std::size_t initialIndex = 0)
        : values(values), currentIndex(initialIndex) {}
    [[nodiscard]] constexpr T current() const {
        return this->values[this->currentIndex];
    }
    [[nodiscard]] constexpr T advance() {
        this->currentIndex = (this->currentIndex + 1) % this->values.size();
        return this->current();
    }
};

#endif