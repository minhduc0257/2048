#include <array>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <string>
#include <SFML/Audio.hpp>
#include "SFML/Graphics.hpp"
#include "lib2048core.hpp"
#include "lib2048utils.hpp"

int cellOutlineThickness = 1;
std::unordered_map<sf::Keyboard::Key, gameMovement> MOVEMENTS
{
    std::make_pair(sf::Keyboard::Up, gameMovement::Up),
    std::make_pair(sf::Keyboard::Right, gameMovement::Right),
    std::make_pair(sf::Keyboard::Down, gameMovement::Down),
    std::make_pair(sf::Keyboard::Left, gameMovement::Left)
};
sf::Keyboard::Key RESTART_KEY = sf::Keyboard::R;

sf::Clock globalClock;
sf::Time previousFrameTime, currentFrameTime;
sf::Font robotoMono, montserratRegular, latoBold;
sf::Sound keyClicked; sf::SoundBuffer _keyClicked;
gameConfig config;

void initializeGlobals()
{
    robotoMono.loadFromFile("./RobotoMono-Regular.ttf");
    montserratRegular.loadFromFile("./Montserrat-Regular.ttf");
    latoBold.loadFromFile("./Lato-Bold.ttf");
    _keyClicked.loadFromFile("./soft-hitsoft.wav");
    keyClicked = sf::Sound(_keyClicked);

    previousFrameTime = globalClock.getElapsedTime();

    // color configuration
    config.setup();
}

sf::Color getCellColor(gameValue value)
{
    sf::Color _;
    if (config.cellColorMapping.count(value)) _ = config.cellColorMapping[value];
    else _ = sf::Color(0xFF, 0xFF, 0xFF, 160);
    if (value) _.a = 180 + (uint8_t) ((log2(value) / log2(2048)) * 75);
    return _;
}

sf::Color getTextColor(gameValue value)
{
    if (config.cellColorMapping.count(value)) return config.textColorMapping[value];
    return sf::Color(0, 0, 0);
}

sf::Texture renderCell(gameValue value, unsigned int cellSide, unsigned int fontSize)
{
    sf::RenderTexture cell;
    unsigned int renderCellSide = cellSide + cellOutlineThickness * 2;
    cell.create(renderCellSide, renderCellSide);
    cell.clear();

    sf::RectangleShape box;
    box.setPosition(0 + cellOutlineThickness, 0 + cellOutlineThickness); // the outline seems to be drawn around the box itself, and not counted in the main rectangle
    box.setOutlineColor(sf::Color(255, 255, 255, 255));
    box.setOutlineThickness(cellOutlineThickness);
    box.setSize(sf::Vector2f(cellSide, cellSide));
    box.setFillColor(getCellColor(value));
    cell.draw(box);

    // render the text
    sf::Text text (value ? std::to_string(value) : "", montserratRegular, fontSize);
    auto textBoundaryBox = text.getGlobalBounds();
    // place the number in the center
    text.setOrigin(
        (textBoundaryBox.width - cellSide) / 2 + textBoundaryBox.left,
        (textBoundaryBox.height - cellSide) / 2 + textBoundaryBox.top
    );
    text.setFillColor(getTextColor(value));

    cell.draw(text);
    cell.display();

    return cell.getTexture();
}

sf::Texture renderScore(gameValue score, unsigned int width, unsigned int height)
{
    sf::RenderTexture _score;
    _score.create(width + cellOutlineThickness * 2, height + cellOutlineThickness * 2);
    _score.clear(sf::Color::Transparent);

    sf::RectangleShape box;
    box.setPosition(0 + cellOutlineThickness, 0 + cellOutlineThickness);
    box.setOutlineColor(sf::Color::Black);
    box.setOutlineThickness(cellOutlineThickness);
    box.setSize(sf::Vector2f(width, height));
    box.setFillColor(sf::Color::Transparent);
    _score.draw(box);

    // render the text
    sf::Text text (std::to_string(score), latoBold, 40);
    auto textBoundaryBox = text.getGlobalBounds();
    text.setOrigin(
        (textBoundaryBox.width - width) / 2 + textBoundaryBox.left,
        (textBoundaryBox.height - height) / 2 + textBoundaryBox.top
    );
    text.setFillColor(sf::Color::Black);

    _score.draw(text);
    _score.display();

    return _score.getTexture();
}

void entry()
{
    // initialize window
    auto desktopMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(sf::VideoMode(desktopMode.width * 4 / 5, desktopMode.height * 4 / 5), "2048");

    gameState game (4); game.initialize();

    // don't overload machines
    window.setFramerateLimit(120);
    window.setTitle("2048");

    bool hasKeyPressed = true;
    sf::Sound(_keyClicked).play();

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event)) if (event.type == sf::Event::Closed) window.close();

        /**
         * Resize the view port should the window get resized
         */
        if (event.type == sf::Event::Resized)
            window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
        window.clear(sf::Color(0xd6d5d200));
        auto windowSize = window.getSize();

        /**
         * Handle keys
         */
        auto __currentKeyState = std::find_if(
            MOVEMENTS.begin(), MOVEMENTS.end(),
            [](std::pair<sf::Keyboard::Key, gameMovement> _) { return sf::Keyboard::isKeyPressed(_.first); }
        );
        bool __validKey = __currentKeyState != MOVEMENTS.end();
        if (__validKey && !hasKeyPressed)
        {
            game.handleMove(MOVEMENTS[__currentKeyState->first]);
            if (keyClicked.getStatus() != keyClicked.Playing) keyClicked.play();
        }
        hasKeyPressed = __validKey;

        /**
         * FPS counter
         */
        currentFrameTime = globalClock.getElapsedTime();
        float fps = 1.0f / (currentFrameTime.asSeconds() - previousFrameTime.asSeconds());
        previousFrameTime = currentFrameTime;
        sf::Text fpsCounter (std::to_string(long(ceil(fps))) + " FPS", robotoMono, 14);
        fpsCounter.setPosition(
            std::min<unsigned int>(windowSize.x / 100, 5),
            (windowSize.y * 99.5f / 100) - fpsCounter.getGlobalBounds().height - fpsCounter.getGlobalBounds().top - 1
        );
        fpsCounter.setFillColor(sf::Color::Black);
        window.draw(fpsCounter);

        /**
         * Game field
         *
         * The game field will be centered in the window horizontally.
         * Width and height should be 75% of the window width/height.
         * Should they be different, the minimum of two will be used.
         */
        auto& matrix = game.matrix;
        unsigned int
            baseDimension = std::min(windowSize.x, windowSize.y),
            matrixSide = baseDimension / 4 * 3;
        unsigned int
            baseX = (windowSize.x - matrixSide) >> 1, baseY = (windowSize.y - matrixSide) >> 1,
            cellSide = matrixSide / matrix[0].size(),
            borderSize = cellSide / 8;
            cellSide -= borderSize;
        for (auto rowIndex = 0 ; rowIndex < matrix.size() ; rowIndex++)
            for (auto cellIndex = 0 ; cellIndex < matrix[rowIndex].size() ; cellIndex++)
            {
                auto cell = renderCell(matrix[rowIndex][cellIndex], cellSide, baseDimension / 20);
                sf::Sprite cellSprite (cell);
                auto renderCellSide = cell.getSize().x;
                cellSprite.setPosition(baseX + cellIndex * (renderCellSide + borderSize), baseY + rowIndex * (renderCellSide + borderSize));
                window.draw(cellSprite);
            }


        /**
         * Score
         */
        auto __ = renderScore(game.score, windowSize.x / 10 * 2, windowSize.y / 20 * 2);
        sf::Sprite score (__);
        score.setPosition(windowSize.x / 2 - score.getGlobalBounds().width / 2, windowSize.y / 15 - score.getGlobalBounds().height / 2);
        window.draw(score);

        window.display();
    }

}

int main()
{
    initializeGlobals();
    entry();
}