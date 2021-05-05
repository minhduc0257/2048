#include "lib2048core.hpp"
#include "lib2048utils.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>

gameState::gameState(gameSize size)
    : size(size),
      random(std::chrono::system_clock::now().time_since_epoch().count()),
      matrix(size) {
    for (auto &row : this->matrix)
        row.assign(this->size, 0);
}

void gameState::initialize() {
    this->lost = false;
    this->score = 0;

    auto cellInitNbr = this->size >> 1;
    std::size_t count = 0;
    while (cellInitNbr != 0 && this->newCell()) {
        --cellInitNbr;
    }
}

bool gameState::newCell() {
    std::vector<decltype(this->matrix[0].begin())> positions;
    std::for_each(this->matrix.begin(), this->matrix.end(),
                  [&positions](auto &vec) {
                      for (auto it = vec.begin(); it != vec.end(); ++it) {
                          if (*it == 0)
                              positions.push_back(it);
                      }
                  });
    if (positions.empty())
        return false;
    auto ranIdx = this->random() % positions.size();
    *positions[ranIdx] = this->generate();
    return true;
}

constexpr std::size_t gameState::count() {
    return std::accumulate(
        this->matrix.begin(), this->matrix.end(), 0,
        [](std::size_t base, const auto &_) {
            return base +
                   std::count_if(_.begin(), _.end(),
                                 [](gameValue value) -> bool { return value; });
        });
}

constexpr gameValue gameState::generate() {
    return (this->random() % 10) > 8 ? 4 : 2;
}

template <typename Iterator>
constexpr std::pair<bool, gameValue> __merge(const Iterator &begin,
                                             const Iterator &end) {
    auto _swapper = begin;
    auto _bound = begin;
    gameValue out = 0;
    bool changed = false;
    for (auto iter = begin; iter != end; ++iter) {
        auto should_swap = *iter != 0 && iter != _swapper;
        if (should_swap) {
            changed = true;
            std::iter_swap(iter, _swapper);
        }
        if (_swapper != _bound && *_swapper == *(_swapper - 1)) {
            *(_swapper - 1) *= 2;
            out += *(_swapper - 1);
            *_swapper = 0;
            changed = true;
            _bound = _swapper;
        }
        if (*_swapper != 0) {
            ++_swapper;
        }
    }
    return {changed, out};
}

diff gameState::handleMove(gameMovement move) {
    bool changed = false;
    if (this->lost)
        return {false, false};
    switch (move) {
    case gameMovement::Up:
    case gameMovement::Down: {
        for (gameSize columnIndex = 0; columnIndex < this->matrix.size();
             ++columnIndex) {
            std::vector<gameValue> column(this->matrix.size());
            std::transform(
                this->matrix.begin(), this->matrix.end(), column.begin(),
                [&columnIndex](const auto &row) { return row[columnIndex]; });

            auto merged = (move == gameMovement::Down)
                              ? __merge(column.rbegin(), column.rend())
                              : __merge(column.begin(), column.end());
            changed = changed || merged.first;

            // Reassign cross value into the matrix
            for (gameSize i = 0; i < column.size(); i++)
                this->matrix[i][columnIndex] = column[i];
            this->score += merged.second;
        }
        break;
    }

    case gameMovement::Left:
    case gameMovement::Right: {
        for (auto &row : this->matrix) {
            auto merged = (move == gameMovement::Right)
                              ? __merge(row.rbegin(), row.rend())
                              : __merge(row.begin(), row.end());
            changed = changed || merged.first;
            this->score += merged.second;
        }
        break;
    }

    case gameMovement::META_Restart: {
        this->initialize();
        return {true, true};
    }

    case gameMovement::META_RandomlyGenerate: {
        return {false, this->newCell()};
    }
    }

    bool generated = false;

    if (changed)
        if (this->count() != this->matrix.size() * this->matrix.size())
            for (auto i = this->size >> 2; i; i--)
                generated = this->newCell();

    this->lost = this->checkLosingState();

    return {changed, generated};
}

constexpr bool gameState::checkLosingState() {
    auto size = this->size;
    for (gameSize row = 0; row < size; row++)
        for (gameSize column = 0; column < size; column++) {
            auto _ = this->matrix[row][column];
            if (!_)
                return false;

            if ((row > 0) && (this->matrix[row - 1][column] == _ ||
                              this->matrix[row - 1][column] == 0))
                return false;
            if ((row + 1 < size) && (this->matrix[row + 1][column] == _ ||
                                     this->matrix[row + 1][column] == 0))
                return false;
            if ((column > 0) && (this->matrix[row][column - 1] == _ ||
                                 this->matrix[row][column - 1] == 0))
                return false;
            if ((column + 1 < size) && (this->matrix[row][column + 1] == _ ||
                                        this->matrix[row][column + 1] == 0))
                return false;
        }
    return true;
}

std::map<gameValue, sf::Color> loadColorMapping(std::string path);
void gameConfig::setup() {
    this->cellColorMapping = loadColorMapping("./color.txt");
    this->textColorMapping = loadColorMapping("./color2.txt");
}

std::map<gameValue, sf::Color> loadColorMapping(std::string path) {
    std::stringstream buf;
    buf << (std::ifstream(path)).rdbuf();
    // Keep the string alive
    auto const &lit_copy = buf.str();
    auto lines = split(lit_copy, "\n");
    // TODO: Convert to ranges::transform
    std::transform(lines.begin(), lines.end(), lines.begin(), trim);
    std::erase_if(lines, [](const auto &s) { return !s.length(); });
    std::map<gameValue, sf::Color> ____;
    std::for_each(
        lines.begin(), lines.end(), [&____, &lit_copy](const auto &s) {
            auto _ = split(s, "=");
            std::transform(_.begin(), _.end(), _.begin(), trim);
            long long cap;
            auto conv_res =
                std::from_chars(_[0].data(), _[0].data() + _[0].size(), cap);
            if (conv_res.ec != std::errc() || _.size() != 2) {
                throw std::exception();
            }
            auto colors = split(_[1], ",");
            std::vector<long> __(colors.size());
            std::transform(colors.begin(), colors.end(), __.begin(),
                           [](const auto &s) -> long {
                               auto trimmed = trim(s);
                               long num;
                               auto result = std::from_chars(
                                   trimmed.data(),
                                   trimmed.data() + trimmed.size(), num, 16);
                               if (result.ec == std::errc()) {
                                   return num;
                               }
                               return 0;
                           });
            ____[cap] = sf::Color(__[0], __[1], __[2],
                                  colors.size() >= 4 ? __[3] : 196);
        });
    return ____;
}