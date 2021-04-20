#include <random>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <chrono>
#include <SFML/Graphics.hpp>

#ifndef _2048CORE
#define _2048CORE

typedef std::int64_t gameValue;
typedef std::size_t gameSize;

enum gameMovement
{
    Up = 1, Right, Down, Left, META_Restart, META_RandomlyGenerate
};

class gameConfig
{
    public:
        std::map<gameValue, sf::Color> cellColorMapping;
        std::map<gameValue, sf::Color> textColorMapping;
        void setup();
};

struct diff { bool changedByUserInteraction; bool generated; };

class gameState
{
    public:
        std::vector<std::vector<gameValue>> matrix;
        gameValue score;
        bool lost;
        gameState(gameSize size);
        void initialize();
        diff handleMove(gameMovement);
    private:
        bool checkLosingState();
        gameSize size;
        std::mt19937_64 random;
        gameValue generate();
        bool newCell();
        std::size_t count();
};

#endif