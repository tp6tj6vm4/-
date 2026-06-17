#ifndef GRID_CELL_H
#define GRID_CELL_H

// 遊戲狀態列舉
enum class GameState {
    Ready,      // 等待玩家點擊第一格
    Playing,    // 遊戲進行中
    Won,        // 勝利
    Lost        // 踩雷失敗
};

// 單一格子屬性
struct GridCell {
    bool isMine = false;
    bool isOpened = false;
    bool isFlagged = false;
    int neighborMines = 0;
};

#endif
