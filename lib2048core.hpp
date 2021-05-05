#include <SFML/Graphics.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <random>
#include <string>
#include <vector>

#ifndef _2048CORE
#define _2048CORE

typedef std::int64_t gameValue;
typedef std::size_t gameSize;

enum class gameMovement {
    Up = 1,
    Right,
    Down,
    Left,
    META_Restart,
    META_RandomlyGenerate
};

class gameConfig {
  public:
    std::map<gameValue, sf::Color> cellColorMapping;
    std::map<gameValue, sf::Color> textColorMapping;
    void setup();
};

struct diff {
    bool changedByUserInteraction;
    bool generated;
};

class gameState {
  public:
    std::vector<std::vector<gameValue>> matrix;
    gameValue score;
    bool lost;
    gameState(gameSize);
    void initialize();
    diff handleMove(gameMovement);

  private:
    constexpr bool checkLosingState();
    gameSize size;
    std::mt19937_64 random;
    constexpr gameValue generate();
    bool newCell();
    constexpr std::size_t count();
};

#endif