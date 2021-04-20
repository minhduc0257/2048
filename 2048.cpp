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
int notificationTimeMsec = 2000;
int cooldownMsec = 5000;
bool hasGameKeyPressed = true;
bool muted = false;
bool paused = false;
bool hasActionKeyPressed = true;
CyclingValues allowedFPS (std::vector<int> { 0, 60, 120, 240, 480 }, 1);
CyclingValues allowedBoardSizes (std::vector<int> { 4, 8, 16 }, 0);

std::unordered_map<sf::Keyboard::Key, gameAction> ACTIONS
{
    std::make_pair(sf::Keyboard::M, gameAction::Mute),
    std::make_pair(sf::Keyboard::Escape, gameAction::Pause),
    // std::make_pair(sf::Keyboard::L, gameAction::Lose)
    std::make_pair(sf::Keyboard::F7, gameAction::CycleFPS),
    std::make_pair(sf::Keyboard::N, gameAction::ResizeGame)
};

std::unordered_map<sf::Keyboard::Key, gameMovement> MOVEMENTS
{
    std::make_pair(sf::Keyboard::Up, gameMovement::Up),
    std::make_pair(sf::Keyboard::Right, gameMovement::Right),
    std::make_pair(sf::Keyboard::Down, gameMovement::Down),
    std::make_pair(sf::Keyboard::Left, gameMovement::Left),
    std::make_pair(sf::Keyboard::R, gameMovement::META_Restart)
};

std::unordered_map<gameValue, sf::Texture> cellTextures;
std::pair<gameValue, sf::Texture> scoreTexture = std::make_pair(-1, sf::Texture());
std::pair<std::string, sf::Texture> pausingScreen;

sf::Clock globalClock;
sf::Time lastNotificationTime; std::string notification;
sf::Time lastKeyPressedTime; gameMovement lastMovement = gameMovement::Up;
sf::Time lastGeneratingKeyPressedTime, lastPause;
sf::Time previousFrameTime, currentFrameTime;
sf::Font robotoMono, montserratRegular, latoBold;
sf::Sound keyClicked; sf::SoundBuffer _keyClicked;
sf::Sound keyClickedFail; sf::SoundBuffer _keyClickedFail;
sf::Sound cooldownGenerated; sf::SoundBuffer _cooldownGenerated;
sf::Music backgroundMusic;
gameConfig config;

void initializeGlobals()
{
    robotoMono.loadFromFile("./RobotoMono-Regular.ttf");
    montserratRegular.loadFromFile("./Montserrat-Regular.ttf");
    latoBold.loadFromFile("./Lato-Bold.ttf");
    _keyClicked.loadFromFile("./soft-hitsoft.wav");
    _keyClickedFail.loadFromFile("./sectionfail.wav");
    _cooldownGenerated.loadFromFile("./sectionpass.wav");
    keyClicked = sf::Sound(_keyClicked);
    keyClickedFail = sf::Sound(_keyClickedFail);
    cooldownGenerated = sf::Sound(_cooldownGenerated);
    backgroundMusic.openFromFile("./pause-loop.wav");
    backgroundMusic.setVolume(50);

    previousFrameTime = globalClock.getElapsedTime();

    // color configuration
    config.setup();
}

inline std::string getGameStatusString(gameState game) { return game.lost ? "LOST" : "PAUSED"; }
void renderPausingScreen(sf::Vector2f windowSize, gameState game)
{
    sf::RenderTexture pausingScreenTexture;
    pausingScreenTexture.create(windowSize.x, windowSize.y);
    pausingScreenTexture.clear(sf::Color::Transparent);
    sf::RectangleShape _;
    _.setSize(sf::Vector2f(windowSize));
    _.setFillColor(sf::Color(0xFF, 0xFF, 0xFF, 230));
    _.setPosition(0, 0);


    sf::Text pausedText (getGameStatusString(game), latoBold, 50);
    auto textBoundaryBox = pausedText.getGlobalBounds();
    pausedText.setPosition(windowSize.x / 2 - textBoundaryBox.width / 2, windowSize.y / 2 - textBoundaryBox.height / 2);
    pausedText.setFillColor(sf::Color::Black);

    pausingScreenTexture.draw(_);
    pausingScreenTexture.draw(pausedText);
    pausingScreenTexture.display();
    pausingScreen = std::make_pair(getGameStatusString(game), pausingScreenTexture.getTexture());
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

sf::Texture renderCell(gameValue value, unsigned int cellSide, float fontSize)
{
    if (cellTextures.count(value)) return cellTextures[value];

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

    return cellTextures[value] = cell.getTexture();
}

sf::Texture renderScore(gameValue score, unsigned int width, unsigned int height)
{
    if (score == scoreTexture.first) return scoreTexture.second;

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

    scoreTexture = std::make_pair(score, _score.getTexture());

    return _score.getTexture();
}

void entry()
{
    // initialize window
    auto desktopMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(sf::VideoMode(desktopMode.width * 4 / 5, desktopMode.height * 4 / 5), "2048");

    gameState game (allowedBoardSizes.current()); game.initialize();

    bool hasFocus = true;

    // don't overload machines
    window.setFramerateLimit(120);
    window.setTitle("2048");

    sf::Sound(_keyClicked).play();
    renderPausingScreen(sf::Vector2f(window.getSize()), game);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::GainedFocus) hasFocus = true;
            if (event.type == sf::Event::LostFocus) { hasFocus = false; paused = true; };
        };

        auto currentTime = globalClock.getElapsedTime();

        /**
         * Resize the view port should the window get resized
         */
        if (event.type == sf::Event::Resized)
        {
            notification = std::string("Resizing viewport ")
                + "to " + std::to_string(event.size.width) + " " + std::to_string(event.size.height);
            lastNotificationTime = currentTime;
            window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
            cellTextures.clear();
            scoreTexture = std::make_pair(-1, sf::Texture());
            renderPausingScreen(sf::Vector2f(window.getSize()), game);
        }
        window.clear(sf::Color(0xd6d5d200));
        auto windowSize = window.getSize();

        if (pausingScreen.first != getGameStatusString(game)) renderPausingScreen(sf::Vector2f(window.getSize()), game);

        backgroundMusic.setLoop(true);
        if (backgroundMusic.getStatus() != sf::SoundSource::Status::Playing) backgroundMusic.play();
        backgroundMusic.setVolume((muted || paused) ? 0 : 100);
        keyClicked.setVolume((muted || paused) ? 0 : 100);
        keyClickedFail.setVolume((muted || paused) ? 0 : 100);

        bool slowDown = true;

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
                slowDown = true;
                auto _ = ACTIONS[__currentActionKey->first];
                switch (_) {
                    case gameAction::Mute: {
                        muted = !muted;
                        break;
                    }
                    case gameAction::Pause: {
                        paused = !paused;
                        slowDown = false;
                        break;
                    }
                    case gameAction::Lose: {
                        game.lost = !game.lost;
                        slowDown = false;
                        break;
                    }
                    case gameAction::ResizeGame: {
                        cellTextures.clear();
                        allowedBoardSizes.advance();
                        game = gameState (allowedBoardSizes.current());
                        game.initialize();
                        slowDown = false;
                        lastGeneratingKeyPressedTime = currentTime;
                        break;
                    }
                    case gameAction::CycleFPS: {
                        allowedFPS.advance();
                        auto fps = allowedFPS.current();
                        window.setFramerateLimit(fps);
                        notification = "Setting framerate limit to " + (fps ? std::to_string(fps) + "FPS" : "unlimited") + ".";
                        lastNotificationTime = currentTime;
                        slowDown = false;
                    }
                }
            }
            hasActionKeyPressed = __validActionKey;

            auto __currentGameKey = std::find_if(
                MOVEMENTS.begin(), MOVEMENTS.end(),
                [](std::pair<sf::Keyboard::Key, gameMovement> _) { return sf::Keyboard::isKeyPressed(_.first); }
            );
            bool __validKey = __currentGameKey != MOVEMENTS.end();

            if (__validKey && !hasGameKeyPressed)
            {
                auto move = MOVEMENTS[__currentGameKey->first];
                if (move == gameMovement::META_Restart) paused = false;
                if (!paused)
                {
                    slowDown = false;
                    auto changed = game.handleMove(move);

                    if (changed.changedByUserInteraction)
                    {
                        keyClicked.play();
                        lastKeyPressedTime = currentTime;
                        lastMovement = move;
                    }
                    else
                        keyClickedFail.play();
                    if (changed.generated) lastGeneratingKeyPressedTime = currentTime;
                }
            }
            hasGameKeyPressed = __validKey;

        }

        auto lastKeyElapsedMsec = currentTime.asMilliseconds() - lastKeyPressedTime.asMilliseconds();
        auto paddedScanTimeMsec = (1 + scanWidthMultiplier) * scanTimeMsec;

        auto currentMsec = currentTime.asMilliseconds();
        if (slowDown
            // has notification ongoing?
            && (lastNotificationTime.asMilliseconds() + notificationTimeMsec > currentMsec)
            // has key animation ?
            && !(lastKeyPressedTime.asMilliseconds() && lastKeyElapsedMsec < paddedScanTimeMsec)
            && (paused || game.lost) && !hasFocus
        )
            continue;

        if (paused || game.lost)
        {
            if (!lastPause.asMilliseconds()) lastPause = currentTime;
            window.draw(sf::Sprite(pausingScreen.second));
        }
        else
        {
            auto pausedMsec = lastPause.asMilliseconds() ? currentTime.asMilliseconds() - lastPause.asMilliseconds() : 0;
            lastGeneratingKeyPressedTime = sf::milliseconds(lastGeneratingKeyPressedTime.asMilliseconds() + pausedMsec);
            lastPause = sf::Time::Zero;
            auto diff = currentTime.asMilliseconds() - lastGeneratingKeyPressedTime.asMilliseconds();
            if (cooldownMsec > diff)
            {
                auto height = float(windowSize.y) * (cooldownMsec - diff) / cooldownMsec;
                sf::RectangleShape cooldown (sf::Vector2f(windowSize.x, height));
                auto fill = sf::Color::White; fill.a = 103;
                cooldown.setFillColor(fill);
                cooldown.setPosition(0, windowSize.y - height);
                window.draw(cooldown);
            }
            else
            {
                if (game.handleMove(gameMovement::META_RandomlyGenerate).generated)
                {
                    lastGeneratingKeyPressedTime = currentTime;
                    cooldownGenerated.play();
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
                    auto cell = renderCell(matrix[rowIndex][cellIndex], cellSide, float(cellSide) / 2.5);
                    sf::Sprite cellSprite (cell);
                    auto renderCellSide = cell.getSize().x;
                    cellSprite.setPosition(baseX + cellIndex * (renderCellSide + borderSize), baseY + rowIndex * (renderCellSide + borderSize));
                    window.draw(cellSprite);
                }


            if (lastKeyPressedTime.asMilliseconds() && lastKeyElapsedMsec < paddedScanTimeMsec)
            {
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
        }

        /**
         * Score
         */
        auto __ = renderScore(game.score, windowSize.x / 10 * 2, windowSize.y / 20 * 2);
        sf::Sprite score (__);
        score.setPosition(windowSize.x / 2 - score.getGlobalBounds().width / 2, windowSize.y / 15 - score.getGlobalBounds().height / 2);
        window.draw(score);

        /**
         * Notification
         */
        auto notifyAppearancePercentage =
            float(currentTime.asMilliseconds() - lastNotificationTime.asMilliseconds())
            / notificationTimeMsec;
        if (notifyAppearancePercentage <= 1) {
            sf::Text notify (notification, robotoMono, 14);
            notify.setPosition(
                std::min<unsigned int>(windowSize.x / 100, 5),
                std::min<unsigned int>(windowSize.y / 100, 5)
            );
            auto notifyColor = sf::Color::Black;
            notifyColor.a = notifyAppearancePercentage > 1 ? 0 : float(1 - notifyAppearancePercentage) * 255;
            notify.setFillColor(notifyColor);
            window.draw(notify);
        }

        window.display();
    }

}

int main()
{
    initializeGlobals();
    entry();
}