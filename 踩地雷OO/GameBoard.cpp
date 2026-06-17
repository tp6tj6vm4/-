#include "GameBoard.h"
#include <cstdlib>
#include <cmath>

GameBoard::GameBoard(int r, int c, int mines) {
    reset(r, c, mines);
}

void GameBoard::reset(int r, int c, int mines) {
    rows = r;
    cols = c;
    totalMines = mines;
    state = GameState::Ready;
    grid.assign(rows, std::vector<GridCell>(cols));
}

void GameBoard::placeMines(int firstR, int firstC) {
    int placed = 0;
    while (placed < totalMines) {
        int r = rand() % rows;
        int c = rand() % cols;
        bool inSafetyZone = (abs(r - firstR) <= 1 && abs(c - firstC) <= 1);

        if (!grid[r][c].isMine && !inSafetyZone) {
            grid[r][c].isMine = true;
            placed++;
        }
    }
}

void GameBoard::calculateNumbers() {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (grid[r][c].isMine) continue;
            int count = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int nr = r + i, nc = c + j;
                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && grid[nr][nc].isMine) {
                        count++;
                    }
                }
            }
            grid[r][c].neighborMines = count;
        }
    }
}

void GameBoard::openCell(int r, int c) {
    if (r < 0 || r >= rows || c < 0 || c >= cols || grid[r][c].isOpened || grid[r][c].isFlagged) return;

    grid[r][c].isOpened = true;

    if (grid[r][c].neighborMines == 0 && !grid[r][c].isMine) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                openCell(r + i, c + j);
            }
        }
    }
}

bool GameBoard::quickReveal(int r, int c) {
    if (!grid[r][c].isOpened || grid[r][c].neighborMines == 0) return false;

    int flagCount = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int nr = r + i, nc = c + j;
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && grid[nr][nc].isFlagged) {
                flagCount++;
            }
        }
    }

    bool hitMine = false;
    if (flagCount == grid[r][c].neighborMines) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int nr = r + i, nc = c + j;
                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                    if (!grid[nr][nc].isOpened && !grid[nr][nc].isFlagged) {
                        if (grid[nr][nc].isMine) hitMine = true;
                        else openCell(nr, nc);
                    }
                }
            }
        }
    }
    return hitMine;
}

void GameBoard::checkWin() {
    int openedCount = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (grid[i][j].isOpened && !grid[i][j].isMine) openedCount++;
        }
    }
    if (openedCount == (rows * cols - totalMines)) {
        state = GameState::Won;
    }
}

void GameBoard::revealAllMines() {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (grid[i][j].isMine) grid[i][j].isOpened = true;
        }
    }
}

void GameBoard::toggleFlag(int r, int c) {
    if (!grid[r][c].isOpened) {
        grid[r][c].isFlagged = !grid[r][c].isFlagged;
    }
}