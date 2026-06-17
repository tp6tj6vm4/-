#include "GameView.h"
#include "ScoreManager.h" 
#include <iostream>
#include <string>
#include <vector>

GameView::GameView() {
    if (!texture.loadFromFile("images/MinesweeperTileset.png")) {
        std::cout << "警告：找不到貼圖檔案 images/MinesweeperTileset.png" << std::endl;
    }
    sprite.setTexture(texture);
    sprite.setScale(SCALE, SCALE);

    hasFont = font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
    if (hasFont) {
        statusText.setFont(font);
        statusText.setCharacterSize(38);
        statusText.setStyle(sf::Text::Bold);

        timerText.setFont(font);
        timerText.setCharacterSize(40);
        timerText.setFillColor(sf::Color::Red);
        timerText.setPosition(10.0f, 5.0f);
        timerText.setString("000");

        helpText.setFont(font);
        helpText.setCharacterSize(14); // 稍微縮小字體以容納更多按鍵提示
        helpText.setFillColor(sf::Color::White);
        helpText.setPosition(80.0f, 12.0f);
        // ★ 介面優化：在說明列補上 [H]Hint 提示
        helpText.setString("Keys: [1-3]Difficulty [R]Reset [L]Leaderboard [H]AI Hint");
    }
}

void GameView::render(sf::RenderWindow& window, const GameBoard& board, int elapsedSeconds,
    const std::string& inputText, bool isEnteringName, bool showLeaderboard,
    int hintRow, int hintCol, int hintType) {

    // 排行榜畫面渲染
    if (showLeaderboard) {
        window.clear(sf::Color(40, 40, 40));

        if (hasFont) {
            std::string currentSizeStr = std::to_string(board.getRows()) + "x" + std::to_string(board.getCols());

            sf::Text titleText("🏆 LEADERBOARD (" + currentSizeStr + ") 🏆", font, 28);
            titleText.setFillColor(sf::Color::Yellow);
            titleText.setStyle(sf::Text::Bold);
            titleText.setPosition(window.getSize().x / 2.0f - titleText.getGlobalBounds().width / 2.0f, 15.0f);
            window.draw(titleText);

            std::string headers[] = { "Rank", "Name", "Time", "Size", "Date" };
            float xOffsets[] = { 20.0f, 75.0f, 195.0f, 255.0f, 325.0f };

            for (int h = 0; h < 5; ++h) {
                sf::Text hText(headers[h], font, 16);
                hText.setFillColor(sf::Color::Cyan);
                hText.setStyle(sf::Text::Bold);
                hText.setPosition(xOffsets[h], 65.0f);
                window.draw(hText);
            }

            ScoreManager scoreDb;
            auto topScores = scoreDb.getTopScores(currentSizeStr, 10);

            float startY = 95.0f;
            float rowSpacing = 26.0f;

            for (size_t i = 0; i < topScores.size(); ++i) {
                sf::Text rText("#" + std::to_string(i + 1), font, 15);
                sf::Text nText(topScores[i].name, font, 15);
                sf::Text tText(std::to_string(topScores[i].clearTime) + "s", font, 15);
                sf::Text sText(topScores[i].mapSize, font, 15);
                sf::Text dText(topScores[i].clearDate, font, 15);

                rText.setPosition(xOffsets[0], startY + i * rowSpacing);
                nText.setPosition(xOffsets[1], startY + i * rowSpacing);
                tText.setPosition(xOffsets[2], startY + i * rowSpacing);
                sText.setPosition(xOffsets[3], startY + i * rowSpacing);
                dText.setPosition(xOffsets[4], startY + i * rowSpacing);

                sf::Color rowColor = sf::Color::White;
                if (i == 0)      rowColor = sf::Color(255, 215, 0);
                else if (i == 1) rowColor = sf::Color(192, 192, 192);
                else if (i == 2) rowColor = sf::Color(205, 127, 50);

                rText.setFillColor(rowColor); nText.setFillColor(rowColor);
                tText.setFillColor(rowColor); sText.setFillColor(rowColor); dText.setFillColor(rowColor);

                window.draw(rText); window.draw(nText); window.draw(tText); window.draw(sText); window.draw(dText);
            }

            sf::Text footerText("Press [L] to return to game", font, 13);
            footerText.setFillColor(sf::Color(170, 170, 170));
            footerText.setPosition(window.getSize().x / 2.0f - footerText.getGlobalBounds().width / 2.0f, window.getSize().y - 25.0f);
            window.draw(footerText);
        }

        window.display();
        return;
    }

    // 遊戲網格渲染
    window.clear(sf::Color(100, 100, 100));
    int blockSize = W * static_cast<int>(SCALE);

    for (int i = 0; i < board.getRows(); i++) {
        for (int j = 0; j < board.getCols(); j++) {
            int tileX = 1, tileY = 2;
            const GridCell& cell = board.getCell(i, j);

            if (cell.isOpened) {
                if (cell.isMine) {
                    tileX = 4; tileY = 1;
                }
                else {
                    int n = cell.neighborMines;
                    if (n == 0) { tileX = 3; tileY = 1; }
                    else if (n >= 1 && n <= 5) { tileX = n - 1; tileY = 0; }
                    else if (n >= 6 && n <= 8) { tileX = n - 6; tileY = 1; }
                }
            }
            else if (cell.isFlagged) {
                tileX = 0; tileY = 2;
            }

            sprite.setTextureRect(sf::IntRect(tileX * W, tileY * W, W, W));
            sprite.setPosition(j * blockSize, i * blockSize + UI_HEIGHT);
            window.draw(sprite);
        }
    }

    // =================================================================
    // ★ 新增：在格子上層疊加渲染 AI 演算法的高亮提示圖層
    // =================================================================
    if (hintType != 0 && hintRow >= 0 && hintRow < board.getRows() && hintCol >= 0 && hintCol < board.getCols()) {
        sf::RectangleShape hintOverlay(sf::Vector2f(static_cast<float>(blockSize), static_cast<float>(blockSize)));

        if (hintType == 1) {
            // 100% 安全：亮綠色，透明度設為 130 保持可讀性
            hintOverlay.setFillColor(sf::Color(0, 255, 0, 130));
        }
        else if (hintType == 2) {
            // 100% 絕對是雷：亮紅色
            hintOverlay.setFillColor(sf::Color(255, 0, 0, 130));
        }

        hintOverlay.setPosition(hintCol * blockSize, hintRow * blockSize + UI_HEIGHT);
        window.draw(hintOverlay);
    }
    // =================================================================

    // 繪製計時器與狀態文字
    if (hasFont) {
        std::string timeStr = std::to_string(elapsedSeconds);
        if (elapsedSeconds < 10) timeStr = "00" + timeStr;
        else if (elapsedSeconds < 100) timeStr = "0" + timeStr;
        else if (elapsedSeconds > 999) timeStr = "999";
        timerText.setString(timeStr);

        window.draw(timerText);
        window.draw(helpText);

        GameState state = board.getState();
        if (state == GameState::Won || state == GameState::Lost) {
            if (state == GameState::Won && isEnteringName) {
                statusText.setString("Enter Name:\n" + (inputText.empty() ? "_" : inputText));
                statusText.setFillColor(sf::Color::Yellow);
            }
            else {
                statusText.setString(state == GameState::Lost ? "GAME OVER" : "YOU WIN!");
                statusText.setFillColor(state == GameState::Lost ? sf::Color::Red : sf::Color::Yellow);
            }

            statusText.setPosition(
                window.getSize().x / 2.0f - statusText.getGlobalBounds().width / 2.0f,
                (window.getSize().y + UI_HEIGHT) / 2.0f - statusText.getGlobalBounds().height / 2.0f
            );
            window.draw(statusText);
        }
    }

    window.display();
}