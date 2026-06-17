#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include <SFML/Graphics.hpp>
#include "GameBoard.h"
#include <string> 

class GameView {
private:
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Font font;
    sf::Text statusText;
    sf::Text timerText;
    sf::Text helpText;

    const int W = 16;
    const float SCALE = 3.0f;
    const int UI_HEIGHT = 60;
    bool hasFont;

public:
    GameView();

    // ★ 修改：參數列最後補上 hintRow, hintCol, hintType 接收 AI 計算結果
    void render(sf::RenderWindow& window, const GameBoard& board, int elapsedSeconds,
        const std::string& inputText, bool isEnteringName, bool showLeaderboard,
        int hintRow = -1, int hintCol = -1, int hintType = 0);

    int getUIHeight() const { return UI_HEIGHT; }
    int getBlockPixelSize() const { return W * static_cast<int>(SCALE); }
};

#endif