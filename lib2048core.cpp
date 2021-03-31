#include <algorithm>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include "lib2048core.hpp"
#include "lib2048utils.hpp"

gameState::gameState(gameSize size)
{
    this->size = size;
    this->matrix.resize(size);
    for (auto &row : this->matrix) row.assign(this->size, 0);
}

void gameState::initialize()
{
    this->score = 0;
    this->matrix.resize(this->size);
    for (auto &row : this->matrix) row.assign(this->size, 0);

    if (this->random_source.entropy() == 0)
        LOG("random_device entropy is zero; randomness will be severely limited.")

    const int max = 2; int made = 0;
    while (made < max) made += this->newCell();
}

bool gameState::newCell()
{
    auto _ = this->random_source() % (this->size * this->size);
    if (this->matrix[_ / 4][_ % 4])
        return false;
    else
        return (this->matrix[_ / 4][_ % 4] = this->generate());

}

std::size_t gameState::count()
{
    return std::accumulate(
        this->matrix.begin(), this->matrix.end(),
        0, [](std::size_t base, std::vector<gameValue> _)
        {
            return base +
                std::accumulate(
                    _.begin(), _.end(),
                    0, [](std::size_t base, gameValue value) { return base + !!value; }
                );
        }
    );
}

gameValue gameState::generate()
{
    return (this->random_source() % 10) > 8 ? 4 : 2;
}

std::pair<std::vector<gameValue>, gameValue> __merge(std::vector<gameValue>& array)
{
    gameValue out = 0;
    std::vector<gameValue> elements; elements.reserve(array.size());
    std::vector<gameValue> merged; merged.reserve(array.size());
    for (auto i : array)
        if (i) elements.push_back(i);

    for (auto i = 0 ; i < elements.size() ; i++)
        if ((i + 1) < elements.size() && elements[i + 1] == elements[i])
        {
            merged.push_back(elements[i++] * 2);
            out += elements[i++] * 2;
        }
        else
            merged.push_back(elements[i]);

    while (merged.size() < array.size()) merged.push_back(0);
    return std::make_pair(merged, out);
}

void gameState::handleMove(gameMovement move)
{
    bool changed = false;
    switch (move)
    {
        case gameMovement::Up:
        case gameMovement::Down:
        {
            for (auto columnIndex = 0 ; columnIndex < this->matrix.size() ; columnIndex++)
            {
                std::vector<gameValue> column; column.reserve(this->matrix.size());
                for (auto row : this->matrix) column.push_back(row[columnIndex]);

                if (move == gameMovement::Down) std::reverse(column.begin(), column.end());
                auto merged = __merge(column);
                if (move == gameMovement::Down) std::reverse(column.begin(), column.end());
                changed = changed || (!compareVector(merged.first, column));
                if (move == gameMovement::Down) std::reverse(merged.first.begin(), merged.first.end());

                for (auto i = 0 ; i < merged.first.size() ; i++)
                    this->matrix[i][columnIndex] = merged.first[i];
                this->score += merged.second;
            }
            break;
        }

        case gameMovement::Left:
        case gameMovement::Right:
        {
            for (auto &row : this->matrix)
            {
                if (move == gameMovement::Right) std::reverse(row.begin(), row.end());
                auto merged = __merge(row);
                if (move == gameMovement::Right) std::reverse(row.begin(), row.end());
                changed = changed || (!compareVector(merged.first, row));
                if (move == gameMovement::Right) std::reverse(merged.first.begin(), merged.first.end());
                row = merged.first;
                this->score += merged.second;
            }
            break;
        }

        case gameMovement::META_Restart:
        {
            this->initialize();
            return;
        }
    }

    if (changed)
    if (this->count() != this->matrix.size() * this->matrix.size())
    {
        bool __new = this->newCell();
        while (!__new) __new = this->newCell();
    }
}

bool gameState::lost()
{

}

std::map<gameValue, sf::Color> loadColorMapping(std::string path);
void gameConfig::setup()
{
    this->cellColorMapping = loadColorMapping("./color.txt");
    this->textColorMapping = loadColorMapping("./color2.txt");
}

std::map<gameValue, sf::Color> loadColorMapping(std::string path)
{
    std::stringstream buf; buf << (std::ifstream (path)).rdbuf();
    auto lines = split(buf.str(), "\n");
    std::transform(lines.begin(), lines.end(), lines.begin(), trim);
    std::remove_if(lines.begin(), lines.end(), [](std::string s) { return s.length(); });
    std::map<gameValue, sf::Color> ____;
    std::for_each(lines.begin(), lines.end(), [&____](std::string s) {
        auto _ = split(s, "=");
        std::transform(_.begin(), _.end(), _.begin(), trim);
        auto cap = std::stoll(_[0]); auto colors = split(_[1], ",");
        std::vector<long> __ (colors.size());
        std::transform(colors.begin(), colors.end(), __.begin(), [](std::string s) -> long {
            return std::stol(trim(s), nullptr, 16);
        });
        ____[cap] = sf::Color(__[0], __[1], __[2], colors.size() >= 4 ? __[3] : 196);
    });
    return ____;
}