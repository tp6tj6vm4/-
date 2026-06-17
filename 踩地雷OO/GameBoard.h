#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include <vector>
#include "GridCell.h"

class GameBoard {
private:
    int rows;
    int cols;
    int totalMines;
    std::vector<std::vector<GridCell>> grid;
    GameState state;

public:
    GameBoard(int r, int c, int mines);

    void reset(int r, int c, int mines);
    void placeMines(int firstR, int firstC);
    void calculateNumbers();
    void openCell(int r, int c);
    bool quickReveal(int r, int c);
    void checkWin();
    void revealAllMines();

    // Getters & Setters
    int getRows() const { return rows; }
    int getCols() const { return cols; }
    int getTotalMines() const { return totalMines; }
    GameState getState() const { return state; }
    void setState(GameState s) { state = s; }
    const GridCell& getCell(int r, int c) const { return grid[r][c]; }
    void toggleFlag(int r, int c);
};

#endif
