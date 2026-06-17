#ifndef SCORE_MANAGER_H
#define SCORE_MANAGER_H

#include <string>
#include <vector>
#include "sqlite3.h" 

// 歷史分數的資料結構
struct ScoreRecord {
    int id;
    std::string name;
    int clearTime;
    std::string clearDate;
    std::string mapSize;
};

class ScoreManager {
private:
    sqlite3* db;
    std::string dbPath;

    // 初始化資料庫（建立資料表）
    void initDatabase();

public:
    ScoreManager(const std::string& path = "minesweeper_scores.db");
    ~ScoreManager();

    // 寫入分數：名稱、通關時間(秒)、地圖列數、地圖行數
    bool insertScore(const std::string& name, int clearTime, int rows, int cols);

    // 讀取指定地圖大小的前 N 名排行榜（新增 mapSize 參數）
    std::vector<ScoreRecord> getTopScores(const std::string& mapSize, int limit = 10);
};

#endif