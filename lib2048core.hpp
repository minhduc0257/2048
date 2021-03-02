#include <random>
#include <vector>
#include <string>
#include <cstdint>
#include "lib2048utils.hpp"

typedef std::int64_t gameValue;
typedef std::size_t gameSize;

enum gameMovement
{
    Up, Right, Down, Left
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