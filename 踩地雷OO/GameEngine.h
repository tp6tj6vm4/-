#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp> 
#include "GameBoard.h"
#include "GameView.h"
#include <string>

// ★ 新增：AI 提示的資料結構
struct AIHint {
    int row = -1;
    int col = -1;
    int type = 0; // 0: 無提示, 1: 100% 安全可點擊(綠色), 2: 100% 絕對是地雷(紅色)
};

class GameEngine {
private:
    sf::RenderWindow window;
    GameBoard board;
    GameView view;
    sf::Clock clock;
    int elapsedSeconds;

    // 視窗內輸入名字所需的狀態變數
    std::string playerNameInput;
    bool isEnteringName;
    bool scoreSaved;

    // 控制是否顯示遊戲內排行榜的狀態開關
    bool showLeaderboard;

    // ★ 新增：當前的 AI 提示狀態
    AIHint currentHint;

    // 音訊資源管理變數
    sf::Music bgm;
    sf::SoundBuffer clickBuffer;
    sf::SoundBuffer flagBuffer;
    sf::SoundBuffer explodeBuffer;
    sf::SoundBuffer victoryBuffer;
    sf::Sound clickSound;
    sf::Sound flagSound;
    sf::Sound explodeSound;
    sf::Sound victorySound;

    void processEvents();
    void update();
    void render();
    void resizeWindow();
    void loadAudio();
    void getAIHint(); // ★ 新增：AI 核心約束消去演算法

public:
    GameEngine();
    void run();
};

#endif