#include "GameEngine.h"
#include <ctime>
#include <iostream>  
#include "ScoreManager.h"
#include <vector>

GameEngine::GameEngine() : board(10, 10, 10), view(), elapsedSeconds(0),
playerNameInput(""), isEnteringName(false), scoreSaved(false),
showLeaderboard(false) {
    srand(static_cast<unsigned int>(time(0)));
    currentHint = { -1, -1, 0 }; // 初始化提示
    loadAudio();
    resizeWindow();
}

void GameEngine::loadAudio() {
    if (bgm.openFromFile("audio/bgm.ogg")) {
        bgm.setLoop(true);
        bgm.setVolume(80.f);
        clickSound.setVolume(30.f);
        flagSound.setVolume(10.f);

        bgm.play();
    }
    else {
        std::cout << "警告：無法載入背景音樂 audio/bgm.ogg" << std::endl;
    }

    if (clickBuffer.loadFromFile("audio/click.wav"))     clickSound.setBuffer(clickBuffer);
    if (flagBuffer.loadFromFile("audio/flag.wav"))       flagSound.setBuffer(flagBuffer);
    if (explodeBuffer.loadFromFile("audio/explode.wav")) explodeSound.setBuffer(explodeBuffer);
    if (victoryBuffer.loadFromFile("audio/victory.wav")) victorySound.setBuffer(victoryBuffer);
}

// ★ 新增：AI 局部約束消去演算法 (資工核心邏輯)
void GameEngine::getAIHint() {
    int rows = board.getRows();
    int cols = board.getCols();

    // 遍歷整個地圖，尋找已經翻開且周圍有數字的格子作為「約束起點」
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const auto& cell = board.getCell(r, c);
            if (!cell.isOpened || cell.neighborMines == 0) continue;

            int unopenedCount = 0;
            int flaggedCount = 0;
            std::vector<std::pair<int, int>> unflaggedUnopenedNeighbors;

            // 掃編周圍 8 格（建構局部圖結構）
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = r + dr;
                    int nc = c + dc;

                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                        const auto& neighbor = board.getCell(nr, nc);
                        if (!neighbor.isOpened) {
                            unopenedCount++;
                            if (neighbor.isFlagged) {
                                flaggedCount++;
                            }
                            else {
                                unflaggedUnopenedNeighbors.push_back({ nr, nc });
                            }
                        }
                    }
                }
            }

            // 演算法規則一：局部滿約束定理 (All Mines Rule)
            // 當「未翻開鄰居總數」等於「該格數字」時，代表周圍所有未翻開的格子 100% 全是地雷！
            if (unopenedCount == cell.neighborMines && !unflaggedUnopenedNeighbors.empty()) {
                currentHint = { unflaggedUnopenedNeighbors[0].first, unflaggedUnopenedNeighbors[0].second, 2 };
                return; // 找到第一個邏輯死角，立即回傳提示
            }

            // 演算法規則二：約束釋放定理 (All Safe Rule)
            // 當「周圍已插旗數」等於「該格數字」時，代表這格的數字約束已滿足，剩下所有未插旗的鄰居 100% 絕對安全！
            if (flaggedCount == cell.neighborMines && !unflaggedUnopenedNeighbors.empty()) {
                currentHint = { unflaggedUnopenedNeighbors[0].first, unflaggedUnopenedNeighbors[0].second, 1 };
                return; // 找到確定安全的格子，立即回傳提示
            }
        }
    }
    // 如果全地圖沒有任何一處滿足基礎邏輯推導（可能需要猜測），則不給予提示
    currentHint = { -1, -1, 0 };
}

void GameEngine::resizeWindow() {
    int winWidth = board.getCols() * view.getBlockPixelSize();
    int winHeight = board.getRows() * view.getBlockPixelSize() + view.getUIHeight();
    window.create(sf::VideoMode(winWidth, winHeight), "Minesweeper Ultra OO");

    playerNameInput = "";
    isEnteringName = false;
    scoreSaved = false;
    showLeaderboard = false;
    currentHint = { -1, -1, 0 }; // 重置提示

    explodeSound.stop();
    victorySound.stop();
    if (bgm.getStatus() != sf::SoundSource::Playing) {
        bgm.play();
    }
}

void GameEngine::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void GameEngine::processEvents() {
    sf::Event e;
    while (window.pollEvent(e)) {
        if (e.type == sf::Event::Closed) {
            window.close();
        }

        // 視窗內打字模式控制
        if (isEnteringName) {
            if (e.type == sf::Event::TextEntered) {
                if (e.text.unicode == 13) {
                    if (playerNameInput.empty()) playerNameInput = "Anonymous";

                    ScoreManager scoreDb;
                    scoreDb.insertScore(playerNameInput, elapsedSeconds, board.getRows(), board.getCols());

                    isEnteringName = false;
                    scoreSaved = true;
                    showLeaderboard = true;
                }
                else if (e.text.unicode == 8) {
                    if (!playerNameInput.empty()) {
                        playerNameInput.pop_back();
                        clickSound.play();
                    }
                }
                else if (e.text.unicode < 128 && e.text.unicode >= 32 && playerNameInput.size() < 12) {
                    playerNameInput += static_cast<char>(e.text.unicode);
                    clickSound.play();
                }
            }
            continue;
        }

        // 鍵盤事件控制
        if (e.type == sf::Event::KeyPressed) {
            if (e.key.code == sf::Keyboard::L) {
                showLeaderboard = !showLeaderboard;
                currentHint.type = 0; // 關閉或開啟排行榜時清除提示
                clickSound.play();
            }
            // ★ 新增：按下 H 鍵，即時計算 AI 提示
            else if (e.key.code == sf::Keyboard::H && !showLeaderboard && board.getState() == GameState::Playing) {
                getAIHint();
                clickSound.play();
            }
            else if (!showLeaderboard) {
                if (e.key.code == sf::Keyboard::Num1 || e.key.code == sf::Keyboard::Numpad1) {
                    board.reset(10, 10, 10); resizeWindow(); elapsedSeconds = 0;
                }
                else if (e.key.code == sf::Keyboard::Num2 || e.key.code == sf::Keyboard::Numpad2) {
                    board.reset(16, 16, 40); resizeWindow(); elapsedSeconds = 0;
                }
                else if (e.key.code == sf::Keyboard::Num3 || e.key.code == sf::Keyboard::Numpad3) {
                    board.reset(16, 30, 99); resizeWindow(); elapsedSeconds = 0;
                }
                else if (e.key.code == sf::Keyboard::R) {
                    board.reset(board.getRows(), board.getCols(), board.getTotalMines()); resizeWindow(); elapsedSeconds = 0;
                }
            }
        }

        // 滑鼠地圖互動
        GameState state = board.getState();
        if (!showLeaderboard && state != GameState::Lost && state != GameState::Won && e.type == sf::Event::MouseButtonPressed) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            if (mousePos.y >= view.getUIHeight()) {
                int col = mousePos.x / view.getBlockPixelSize();
                int row = (mousePos.y - view.getUIHeight()) / view.getBlockPixelSize();

                if (col >= 0 && col < board.getCols() && row >= 0 && row < board.getRows()) {

                    // ★ 只要玩家有點擊地圖任何格子，就將前一次的 AI 提示洗掉，維持畫面乾淨
                    currentHint.type = 0;

                    if (e.mouseButton.button == sf::Mouse::Left) {

                        if (board.getState() == GameState::Ready) {
                            board.placeMines(row, col);
                            board.calculateNumbers();
                            board.setState(GameState::Playing);
                            clock.restart();
                        }

                        if (!board.getCell(row, col).isOpened && !board.getCell(row, col).isFlagged) {
                            if (board.getCell(row, col).isMine) {
                                board.setState(GameState::Lost);
                                board.revealAllMines();
                                bgm.stop();
                                explodeSound.play();
                            }
                            else {
                                board.openCell(row, col);
                                board.checkWin();
                                if (board.getState() != GameState::Won) clickSound.play();
                            }
                        }
                        else if (board.getCell(row, col).isOpened) {
                            if (board.quickReveal(row, col)) {
                                board.setState(GameState::Lost);
                                board.revealAllMines();
                                bgm.stop();
                                explodeSound.play();
                            }
                            else {
                                board.checkWin();
                                if (board.getState() != GameState::Won) clickSound.play();
                            }
                        }

                        if (board.getState() == GameState::Won && !scoreSaved) {
                            isEnteringName = true;
                            bgm.stop();
                            victorySound.play();
                        }
                    }
                    else if (e.mouseButton.button == sf::Mouse::Right) {
                        if (!board.getCell(row, col).isOpened) {
                            board.toggleFlag(row, col);
                            flagSound.play();
                        }
                    }
                }
            }
        }
    }
}

void GameEngine::update() {
    if (!showLeaderboard && board.getState() == GameState::Playing) {
        elapsedSeconds = static_cast<int>(clock.getElapsedTime().asSeconds());
    }
}

void GameEngine::render() {
    // ★ 修改：將 currentHint 的 row, col, type 拆開傳遞給 View 進行高亮疊加繪製
    view.render(window, board, elapsedSeconds, playerNameInput, isEnteringName, showLeaderboard,
        currentHint.row, currentHint.col, currentHint.type);
}