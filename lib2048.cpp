#include <algorithm>
#include <queue>
#include <random>
#include <set>
#include <string>
#include "lib2048.hpp"

gameState::gameState(gameSize size)
{
    this->size = size;
    this->matrix.resize(size);
    for (auto &row : this->matrix) row.assign(this->size, 0);
}

void gameState::initialize()
{
    this->matrix.resize(this->size);
    for (auto &row : this->matrix) row.assign(this->size, 0);

    if (this->random_source.entropy() == 0)
        LOG("random_device entropy is zero; randomness will be severely limited.")

    const int max = 2; int made = 0;
    while (made < max) made += this->newCell();
}

bool gameState::newCell()
{
    auto cell_count = this->size * this->size;
    auto _ = this->random_source() % cell_count;
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

std::vector<gameValue> __merge(std::vector<gameValue>& array)
{
    std::vector<gameValue> elements; elements.reserve(array.size());
    std::vector<gameValue> merged; merged.reserve(array.size());
    for (auto i : array)
        if (i) elements.push_back(i);

    for (auto i = 0 ; i < elements.size() ; i++)
        if ((i + 1) < elements.size() && elements[i + 1] == elements[i])
            merged.push_back(elements[i++] * 2);
        else
            merged.push_back(elements[i]);

    while (merged.size() < array.size()) merged.push_back(0);
    return merged;
}

void gameState::handleMove(gameMovement move)
{
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
                column = __merge(column);
                if (move == gameMovement::Down) std::reverse(column.begin(), column.end());

                for (auto i = 0 ; i < column.size() ; i++)
                    this->matrix[i][columnIndex] = column[i];
            }
            break;
        }

        case gameMovement::Left:
        case gameMovement::Right:
        {
            for (auto &row : this->matrix)
            {
                if (move == gameMovement::Right) std::reverse(row.begin(), row.end());
                row = __merge(row);
                if (move == gameMovement::Right) std::reverse(row.begin(), row.end());
            }
            break;
        }
    }

    if (this->count() != this->matrix.size() * this->matrix.size())
    {
        bool __new = this->newCell();
        while (!__new) __new = this->newCell();
    }
}