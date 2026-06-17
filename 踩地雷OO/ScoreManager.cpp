#include "ScoreManager.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

// 輔助函式：獲取當前系統日期時間 (YYYY-MM-DD HH:MM)
std::string getCurrentDateTime() {
    std::time_t t = std::time(nullptr);
    std::tm tm;
#if defined(_MSC_VER)
    localtime_s(&tm, &t); // Windows 安全版本
#else
    tm = *std::localtime(&t);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

ScoreManager::ScoreManager(const std::string& path) : dbPath(path), db(nullptr) {
    // 開啟或建立資料庫檔案
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "無法開啟資料庫: " << sqlite3_errmsg(db) << std::endl;
    }
    else {
        initDatabase();
    }
}

ScoreManager::~ScoreManager() {
    if (db) {
        sqlite3_close(db);
    }
}

void ScoreManager::initDatabase() {
    // SQL: 建立歷史紀錄資料表
    std::string sql =
        "CREATE TABLE IF NOT EXISTS leaderboards ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "clear_time INTEGER NOT NULL,"
        "clear_date TEXT NOT NULL,"
        "map_size TEXT NOT NULL);";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "建立資料表失敗: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

bool ScoreManager::insertScore(const std::string& name, int clearTime, int rows, int cols) {
    // 格式化地圖大小，例如 "16x30"
    std::string mapSizeStr = std::to_string(rows) + "x" + std::to_string(cols);
    std::string dateStr = getCurrentDateTime();

    // 使用 PreparedStatement 防止 SQL 注入
    std::string sql = "INSERT INTO leaderboards (name, clear_time, clear_date, map_size) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    // 綁定參數
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, clearTime);
    sqlite3_bind_text(stmt, 3, dateStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, mapSizeStr.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

// 加上第一個參數 mapSize（例如傳入 "10x10"）
std::vector<ScoreRecord> ScoreManager::getTopScores(const std::string& mapSize, int limit) {
    std::vector<ScoreRecord> results;

    // ★ 修改 SQL：只撈取符合當前地圖大小的紀錄
    std::string sql = "SELECT id, name, clear_time, clear_date, map_size FROM leaderboards WHERE map_size = ? ORDER BY clear_time ASC LIMIT ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        // ★ 綁定地圖大小參數
        sqlite3_bind_text(stmt, 1, mapSize.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ScoreRecord record;
            record.id = sqlite3_column_int(stmt, 0);
            record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            record.clearTime = sqlite3_column_int(stmt, 2);
            record.clearDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            record.mapSize = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            results.push_back(record);
        }
    }
    sqlite3_finalize(stmt);
    return results;
}