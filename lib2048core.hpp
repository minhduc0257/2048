#include <random>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <SFML/Graphics.hpp>

#ifndef _2048CORE
#define _2048CORE

typedef std::int64_t gameValue;
typedef std::size_t gameSize;

enum gameMovement
{
    Up, Right, Down, Left
};

class gameConfig
{
    public:
        std::map<gameValue, sf::Color> cellColorMapping;
        std::map<gameValue, sf::Color> textColorMapping;
        void setup();
};

class gameState
{
    public:
        std::vector<std::vector<gameValue>> matrix;
        gameState(gameSize size);
        void initialize();
        void handleMove(gameMovement);
    private:
        gameSize size;
        std::random_device random_source;
        gameValue generate();
        bool newCell();
        std::size_t count();
};

#endif