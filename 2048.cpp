#include <array>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <SFML/Audio.hpp>
#include "SFML/Graphics.hpp"
#include "lib2048core.hpp"
#include "lib2048utils.hpp"
#include "lib2048ui.hpp"

int cellOutlineThickness = 1;
float scanWidthMultiplier = 0.5;
int scanTimeMsec = 100;
bool hasGameKeyPressed = true;
bool muted = false;
bool paused = false;
bool hasActionKeyPressed = true;
std::unordered_map<sf::Keyboard::Key, gameAction> ACTIONS
{
    std::make_pair(sf::Keyboard::M, gameAction::Mute),
    std::make_pair(sf::Keyboard::Escape, gameAction::Pause),
    std::make_pair(sf::Keyboard::L, gameAction::Lose)
};

std::unordered_map<sf::Keyboard::Key, gameMovement> MOVEMENTS
{
    std::make_pair(sf::Keyboard::Up, gameMovement::Up),
    std::make_pair(sf::Keyboard::Right, gameMovement::Right),
    std::make_pair(sf::Keyboard::Down, gameMovement::Down),
    std::make_pair(sf::Keyboard::Left, gameMovement::Left),
    std::make_pair(sf::Keyboard::R, gameMovement::META_Restart)
};

sf::Clock globalClock;
sf::Time lastKeyPressedTime; gameMovement lastMovement = gameMovement::Up;
sf::Time previousFrameTime, currentFrameTime;
sf::Font robotoMono, montserratRegular, latoBold;
sf::Sound keyClicked; sf::SoundBuffer _keyClicked;
sf::Sound keyClickedFail; sf::SoundBuffer _keyClickedFail;
sf::Music backgroundMusic;
gameConfig config;

void initializeGlobals()
{
    robotoMono.loadFromFile("./RobotoMono-Regular.ttf");
    montserratRegular.loadFromFile("./Montserrat-Regular.ttf");
    latoBold.loadFromFile("./Lato-Bold.ttf");
    _keyClicked.loadFromFile("./soft-hitsoft.wav");
    _keyClickedFail.loadFromFile("./sectionfail.wav");
    keyClicked = sf::Sound(_keyClicked);
    keyClickedFail = sf::Sound(_keyClickedFail);
    backgroundMusic.openFromFile("./pause-loop.wav");
    backgroundMusic.setVolume(50);

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

    bool hasFocus = true;

    // don't overload machines
    window.setFramerateLimit(120);
    window.setTitle("2048");

    sf::Sound(_keyClicked).play();

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::GainedFocus) hasFocus = true;
            if (event.type == sf::Event::LostFocus) { hasFocus = false; paused = true; };
        };

        /**
         * Resize the view port should the window get resized
         */
        if (event.type == sf::Event::Resized)
            window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
        window.clear(sf::Color(0xd6d5d200));
        auto windowSize = window.getSize();

        backgroundMusic.setLoop(true);
        if (backgroundMusic.getStatus() != sf::SoundSource::Status::Playing) backgroundMusic.play();
        backgroundMusic.setVolume((muted || paused) ? 0 : 100);
        keyClicked.setVolume((muted || paused) ? 0 : 100);
        keyClickedFail.setVolume((muted || paused) ? 0 : 100);

        if (hasFocus) {
            /**
             * Handle action keys
             */
            auto __currentActionKey = std::find_if(
                ACTIONS.begin(), ACTIONS.end(),
                [](std::pair<sf::Keyboard::Key, gameAction> _) { return sf::Keyboard::isKeyPressed(_.first); }
            );
            bool __validActionKey = __currentActionKey != ACTIONS.end();
            if (__validActionKey && !hasActionKeyPressed)
            {
                auto _ = ACTIONS[__currentActionKey->first];
                switch (_) {
                    case gameAction::Mute: {
                        muted = !muted;
                        break;
                    }
                    case gameAction::Pause: {
                        paused = !paused;
                        break;
                    }
                    case gameAction::Lose: {
                        game.lost = !game.lost;
                        break;
                    }
                }
            }
            hasActionKeyPressed = __validActionKey;

            if (!paused) {
                /**
                 * Handle keys
                 */
                auto __currentGameKey = std::find_if(
                    MOVEMENTS.begin(), MOVEMENTS.end(),
                    [](std::pair<sf::Keyboard::Key, gameMovement> _) { return sf::Keyboard::isKeyPressed(_.first); }
                );
                bool __validKey = __currentGameKey != MOVEMENTS.end();
                if (__validKey && !hasGameKeyPressed)
                {
                    auto changed = game.handleMove(MOVEMENTS[__currentGameKey->first]);
                    if (changed) {
                        keyClicked.play();
                        lastKeyPressedTime = globalClock.getElapsedTime();
                        lastMovement = MOVEMENTS[__currentGameKey->first];
                    }
                    else
                        keyClickedFail.play();
                }
                hasGameKeyPressed = __validKey;
            }
        }

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

        auto lastKeyElapsedMsec = globalClock.getElapsedTime().asMilliseconds() - lastKeyPressedTime.asMilliseconds();
        auto paddedScanTimeMsec = (1 + scanWidthMultiplier) * scanTimeMsec;
        if (lastKeyPressedTime.asMilliseconds() && lastKeyElapsedMsec < paddedScanTimeMsec) {
            auto scanCoverage = (cellSide + cellOutlineThickness * 2) * matrix.size() + (matrix.size() - 1) * borderSize;
            auto scanWidth = scanCoverage * scanWidthMultiplier;
            bool increment = true; gameSize from = 0, to = scanWidth;
            auto progress = float(scanTimeMsec - lastKeyElapsedMsec) / scanTimeMsec;
            switch (lastMovement) {
                case gameMovement::Down:
                case gameMovement::Right:
                    increment = false; from = scanWidth, to = 0; progress = 1 - progress;
                case gameMovement::Up:
                case gameMovement::Left: {
                    for (auto i = from ; (from > to ? i > to : i < to) ; increment ? i++ : i--) {
                        auto y = baseY
                            + progress * matrixSide
                            + (matrixSide / 2) - scanWidth + 1 + i;
                        auto x = baseX
                            + progress * matrixSide
                            + (matrixSide / 2) - scanWidth + 1 + i;
                        sf::RectangleShape scan;
                        auto c = sf::Color::White;
                        const int maximumAlpha = 128;
                        if (lastMovement == gameMovement::Up || lastMovement == gameMovement::Down) {
                            if (y < baseY || y > baseY + scanCoverage) continue;
                            scan.setSize(sf::Vector2f(scanCoverage, 1));
                            scan.setPosition(baseX, y);
                            c.a = maximumAlpha * ((lastMovement == gameMovement::Up ? 1 - float(i + 1) : float(i + 1)) / scanWidth);
                        }

                        if (lastMovement == gameMovement::Left || lastMovement == gameMovement::Right) {
                            if (x < baseX || x > baseX + scanCoverage) continue;
                            scan.setSize(sf::Vector2f(1, scanCoverage));
                            scan.setPosition(x, baseY);
                            c.a = maximumAlpha * ((lastMovement == gameMovement::Left ? 1 - float(i + 1) : float(i + 1)) / scanWidth);
                        }


                        scan.setFillColor(c);
                        window.draw(scan);
                    }
                }
            };
        }

        /**
         * Score
         */
        auto __ = renderScore(game.score, windowSize.x / 10 * 2, windowSize.y / 20 * 2);
        sf::Sprite score (__);
        score.setPosition(windowSize.x / 2 - score.getGlobalBounds().width / 2, windowSize.y / 15 - score.getGlobalBounds().height / 2);
        window.draw(score);

        if (paused || game.lost) {
            sf::RectangleShape _;
            _.setSize(sf::Vector2f(windowSize));
            _.setFillColor(sf::Color(0xFF, 0xFF, 0xFF, 0xFF / 5 * 4));
            _.setPosition(0, 0);


            sf::Text pausedText (game.lost ? "LOST" : "PAUSED", latoBold, 50);
            auto textBoundaryBox = pausedText.getGlobalBounds();
            pausedText.setPosition(windowSize.x / 2 - textBoundaryBox.width / 2, windowSize.y / 2 - textBoundaryBox.height / 2);
            pausedText.setFillColor(sf::Color::Black);

            window.draw(_);
            window.draw(pausedText);
        }

        /**
         * FPS counter
         */
        currentFrameTime = globalClock.getElapsedTime();
        float fps = 1.0f / (currentFrameTime.asSeconds() - previousFrameTime.asSeconds());
        previousFrameTime = currentFrameTime;
        sf::Text fpsCounter (std::to_string(long(ceil(fps))) + " FPS" + (muted ? " - Muted" : "") + (game.lost ? " - Lost" : ""), robotoMono, 14);
        fpsCounter.setPosition(
            std::min<unsigned int>(windowSize.x / 100, 5),
            (windowSize.y * 99.5f / 100) - fpsCounter.getGlobalBounds().height - fpsCounter.getGlobalBounds().top - 1
        );
        fpsCounter.setFillColor(sf::Color::Black);
        window.draw(fpsCounter);

        window.display();
    }

}

int main()
{
    initializeGlobals();
    entry();
}