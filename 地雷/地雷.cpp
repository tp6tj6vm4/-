#define NOMINMAX // 防止 Windows 系統底層的 max/min 巨集與 C++ 標準庫衝突

#include <iostream>
#include <vector>
#include <cstdlib> // 提供 rand() 與 srand()，用於生成隨機數
#include <ctime>   // 提供 time()，用於設定隨機數種子
#include <string>  // 提供 std::string 與 std::to_string()
#include <SFML/Graphics.hpp> // 引入 SFML 圖形庫

// ==========================================
// 1. 資料結構 (Data Structure)
// ==========================================
// 定義地圖上每一個格子的狀態
struct GridCell {
    bool isMine = false;       // 這格是否藏有地雷
    bool isOpened = false;     // 這格是否已經被玩家翻開
    bool isFlagged = false;    // 這格是否被玩家插上紅旗
    int neighborMines = 0;     // 這格周圍 8 個方向總共有幾顆地雷 (0~8)
};

// ==========================================
// 2. 遊戲主類別 (Minesweeper Class)
// 採用物件導向設計 (OOP)，將所有變數與功能封裝在一起
// ==========================================
class Minesweeper {
private:
    // --- SFML 系統與圖形資源 ---
    sf::RenderWindow window;   // 遊戲主視窗
    sf::Texture texture;       // 儲存整張精靈圖 (Sprite Sheet) 的材質
    sf::Sprite sprite;         // 用來切割並繪製材質的精靈物件
    sf::Font font;             // 遊戲使用的字體 (用於顯示文字)
    sf::Text statusText;       // 顯示 "GAME OVER" 或 "YOU WIN!" 的文字物件
    sf::Text timerText;        // 顯示遊戲時間的文字物件
    sf::Text helpText;         // 顯示操作提示的文字物件
    sf::Clock clock;           // SFML 內建計時器，用於計算遊玩時間

    // --- 遊戲常數設定 ---
    const int W = 16;          // 貼圖中每個方塊的原始像素大小 (16x16)
    const float SCALE = 3.0f;  // 畫面放大倍率 (讓 16px 變成 48px，方便觀看)
    const int UI_HEIGHT = 60;  // 視窗頂部預留給計時器與提示文字的 UI 區塊高度

    // --- 遊戲狀態變數 ---
    int rows, cols;            // 目前地圖的行數(高)與列數(寬)
    int totalMines;            // 目前地圖的地雷總數
    int elapsedSeconds;        // 遊戲經過的秒數
    bool firstClick;           // 記錄玩家是否還沒點下第一格 (第一格絕對安全機制)
    bool gameOver;             // 遊戲是否失敗 (踩到地雷)
    bool gameWon;              // 遊戲是否勝利 (翻開所有安全格)
    bool timerStarted;         // 計時器是否已經開始運作
    bool hasFont;              // 記錄字體是否成功載入

    // --- 核心地圖資料 ---
    // 使用 2D Vector 儲存整個網格，外層是行(y)，內層是列(x)
    std::vector<std::vector<GridCell>> board;

public:
    // ==========================================
    // 建構子 (Constructor)：程式啟動時只會執行一次的初始化作業
    // ==========================================
    Minesweeper() {
        // 設定隨機數種子，確保每次執行遊戲地雷位置都不一樣
        srand(static_cast<unsigned int>(time(0)));

        // 載入遊戲貼圖素材
        if (!texture.loadFromFile("images/MinesweeperTileset.png")) {
            std::cout << "警告：找不到貼圖檔案 images/MinesweeperTileset.png" << std::endl;
        }
        sprite.setTexture(texture);
        sprite.setScale(SCALE, SCALE); // 套用放大倍率

        // 載入系統字體 (此處使用 Windows 內建的 Arial 字體)
        hasFont = font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
        if (hasFont) {
            // 設定勝負文字屬性 (字體、大小、粗體)
            statusText.setFont(font);
            statusText.setCharacterSize(45);
            statusText.setStyle(sf::Text::Bold);

            // 設定計時器文字屬性 (紅字、位置在左上角)
            timerText.setFont(font);
            timerText.setCharacterSize(40);
            timerText.setFillColor(sf::Color::Red);
            timerText.setPosition(10.0f, 5.0f);

            // 設定操作提示文字屬性 (白字、位置在計時器右邊)
            helpText.setFont(font);
            helpText.setCharacterSize(16);
            helpText.setFillColor(sf::Color::White);
            helpText.setPosition(80.0f, 10.0f);
            helpText.setString("Keys: [1]Small [2]Med [3]Large [R]Reset");
        }

        // 遊戲啟動時，預設以「初級地圖 (10x10, 10雷)」開始
        resetGame(10, 10, 10);
    }

    // ==========================================
    // 公開方法：啟動遊戲主迴圈
    // ==========================================
    void run() {
        // 只要視窗沒有被關閉，就持續執行「抓取事件 -> 更新狀態 -> 繪製畫面」的循環
        while (window.isOpen()) {
            processEvents(); // 處理滑鼠與鍵盤輸入
            update();        // 更新計時器等遊戲邏輯
            render();        // 繪製最新畫面
        }
    }

private:
    // ==========================================
    // --- 核心邏輯模組 (Logic Methods) ---
    // ==========================================

    // 重置遊戲狀態，並根據新的長、寬、地雷數重新建立視窗與地圖
    void resetGame(int r, int c, int mines) {
        rows = r;
        cols = c;
        totalMines = mines;

        // 初始化所有狀態變數
        firstClick = true;
        gameOver = false;
        gameWon = false;
        timerStarted = false;
        elapsedSeconds = 0;

        // 重置計時器顯示
        if (hasFont) timerText.setString("000");

        // 重新分配地圖陣列的大小，並將所有格子恢復為預設狀態 (全滿的 GridCell)
        board.assign(rows, std::vector<GridCell>(cols));

        // 重新建立 SFML 視窗以適應新的地圖尺寸 (注意高度要加上 UI_HEIGHT)
        window.create(sf::VideoMode(cols * W * (int)SCALE, rows * W * (int)SCALE + UI_HEIGHT), "Minesweeper Pro");
    }

    // 隨機佈置地雷 (避開玩家點擊的第一格周圍九宮格，保證第一鍵絕對安全)
    void placeMines(int firstR, int firstC) {
        int placed = 0;
        while (placed < totalMines) {
            int r = rand() % rows;
            int c = rand() % cols;

            // 判斷隨機抽到的格子，是否在玩家點擊座標 (firstR, firstC) 的九宮格範圍內
            bool inSafetyZone = (abs(r - firstR) <= 1 && abs(c - firstC) <= 1);

            // 如果該格還沒有地雷，且不在安全區內，就放下一顆地雷
            if (!board[r][c].isMine && !inSafetyZone) {
                board[r][c].isMine = true;
                placed++;
            }
        }
    }

    // 遍歷整個地圖，計算每個非地雷格子周圍 8 個方向的地雷總數
    void calculateNumbers() {
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (board[r][c].isMine) continue; // 如果自己是地雷就不需要計算

                int count = 0;
                // 檢查周圍的 3x3 區域
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        int nr = r + i, nc = c + j;
                        // 確保檢查的座標沒有超出地圖邊界
                        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && board[nr][nc].isMine) {
                            count++;
                        }
                    }
                }
                // 將計算結果存入格子的 neighborMines 屬性中
                board[r][c].neighborMines = count;
            }
        }
    }

    // 遞迴翻開格子 (Flood Fill 演算法)
    void openCell(int r, int c) {
        // 遞迴終止條件：超出邊界、該格已翻開、或者被插了旗子
        if (r < 0 || r >= rows || c < 0 || c >= cols || board[r][c].isOpened || board[r][c].isFlagged) return;

        board[r][c].isOpened = true; // 翻開該格

        // 如果這格周圍沒有地雷 (數字為 0)，則自動向周圍 8 個方向繼續翻開
        if (board[r][c].neighborMines == 0 && !board[r][c].isMine) {
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    openCell(r + i, c + j);
                }
            }
        }
    }

    // 快速展開 / 和弦功能 (Chording)：點擊已翻開的數字格，若周圍旗子數正確，自動展開其餘安全格
    bool quickReveal(int r, int c) {
        // 如果這格還沒被翻開，或是空白格 (0)，就不觸發此功能
        if (!board[r][c].isOpened || board[r][c].neighborMines == 0) return false;

        // 1. 計算周圍 8 格被插了幾根旗子
        int flagCount = 0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int nr = r + i, nc = c + j;
                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && board[nr][nc].isFlagged) {
                    flagCount++;
                }
            }
        }

        // 2. 如果旗子數量剛好等於這格提示的地雷數，就自動翻開周圍未翻開的格子
        bool hitMine = false;
        if (flagCount == board[r][c].neighborMines) {
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int nr = r + i, nc = c + j;
                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                        // 只翻開未翻開且未插旗的格子
                        if (!board[nr][nc].isOpened && !board[nr][nc].isFlagged) {
                            // 如果展開的格子是地雷，代表玩家插錯旗子了，回傳踩雷狀態
                            if (board[nr][nc].isMine) hitMine = true;
                            else openCell(nr, nc); // 否則安全翻開
                        }
                    }
                }
            }
        }
        return hitMine; // 回傳 true 代表觸發了地雷，false 代表安全
    }

    // 檢查遊戲是否勝利：計算已翻開的安全格數量，是否等於總格數扣掉地雷數
    void checkWin() {
        int openedCount = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (board[i][j].isOpened && !board[i][j].isMine) openedCount++;
            }
        }
        if (openedCount == (rows * cols - totalMines)) gameWon = true;
    }

    // 遊戲失敗時呼叫，強制將全地圖所有的地雷翻開顯示
    void revealAllMines() {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (board[i][j].isMine) board[i][j].isOpened = true;
            }
        }
    }

    // ==========================================
    // --- 事件處理與更新模組 (Event & Update) ---
    // ==========================================

    // 處理所有來自使用者的輸入 (鍵盤、滑鼠、視窗關閉)
    void processEvents() {
        sf::Event e;
        while (window.pollEvent(e)) {
            // 視窗右上角的叉叉被點擊
            if (e.type == sf::Event::Closed) window.close();

            // 處理鍵盤按鍵事件 (用於切換難度或重新開始)
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Num1 || e.key.code == sf::Keyboard::Numpad1) resetGame(10, 10, 10);        // 按 1: 初級
                else if (e.key.code == sf::Keyboard::Num2 || e.key.code == sf::Keyboard::Numpad2) resetGame(16, 16, 40);   // 按 2: 中級
                else if (e.key.code == sf::Keyboard::Num3 || e.key.code == sf::Keyboard::Numpad3) resetGame(16, 30, 99);   // 按 3: 高級
                else if (e.key.code == sf::Keyboard::R) resetGame(rows, cols, totalMines);                                 // 按 R: 重新開始
            }

            // 處理滑鼠點擊事件 (僅在遊戲未結束時處理)
            if (!gameOver && !gameWon && e.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                // 確保點擊位置在頂部 UI 橫幅之下，才視為點擊地圖
                if (mousePos.y >= UI_HEIGHT) {
                    // 將像素座標轉換為陣列的 [行][列] 索引
                    int col = mousePos.x / (W * (int)SCALE);
                    int row = (mousePos.y - UI_HEIGHT) / (W * (int)SCALE);

                    // 確保算出來的索引在地圖範圍內
                    if (col >= 0 && col < cols && row >= 0 && row < rows) {
                        handleMouseClick(row, col, e.mouseButton.button); // 呼叫具體的點擊處理邏輯
                    }
                }
            }
        }
    }

    // 負責處理具體的滑鼠左鍵/右鍵邏輯
    void handleMouseClick(int row, int col, sf::Mouse::Button button) {
        if (button == sf::Mouse::Left) { // 滑鼠左鍵

            // 情況 A：點擊未翻開且未插旗的格子
            if (!board[row][col].isOpened && !board[row][col].isFlagged) {
                // 如果是遊戲的第一下點擊，進行佈雷與初始化計時器
                if (firstClick) {
                    placeMines(row, col);
                    calculateNumbers();
                    firstClick = false;
                    clock.restart();
                    timerStarted = true;
                }

                // 判斷是否踩到雷
                if (board[row][col].isMine) {
                    gameOver = true;
                    revealAllMines(); // 踩雷，顯示全圖地雷
                }
                else {
                    openCell(row, col); // 安全，遞迴翻開
                    checkWin();         // 檢查是否達成勝利條件
                }
            }
            // 情況 B：點擊已經翻開的數字格 (觸發快速展開/和弦功能)
            else if (board[row][col].isOpened) {
                if (quickReveal(row, col)) {
                    gameOver = true;    // 若展開過程觸發地雷 (插錯旗)，則失敗
                    revealAllMines();
                }
                else {
                    checkWin();         // 展開安全，檢查是否勝利
                }
            }
        }
        else if (button == sf::Mouse::Right) { // 滑鼠右鍵
            // 只能在未翻開的格子上切換插旗狀態
            if (!board[row][col].isOpened) {
                board[row][col].isFlagged = !board[row][col].isFlagged;
            }
        }
    }

    // 每幀更新遊戲邏輯 (目前僅用於更新計時器)
    void update() {
        if (timerStarted && !gameOver && !gameWon) {
            elapsedSeconds = clock.getElapsedTime().asSeconds();
            if (hasFont) {
                // 將秒數格式化為三位數字串 (例如：005, 042, 120)
                std::string timeStr = std::to_string(elapsedSeconds);
                if (elapsedSeconds < 10) timeStr = "00" + timeStr;
                else if (elapsedSeconds < 100) timeStr = "0" + timeStr;
                else if (elapsedSeconds > 999) timeStr = "999";
                timerText.setString(timeStr);
            }
        }
    }

    // ==========================================
    // --- 渲染模組 (Render) ---
    // ==========================================

    // 負責將所有資料繪製到畫面上
    void render() {
        // 清除上一幀的畫面，填滿深灰色背景
        window.clear(sf::Color(100, 100, 100));

        // 1. 雙層迴圈繪製地圖網格
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                int tileX = 1, tileY = 2; // 預設顯示：未翻開的灰色立體方塊

                // 決定要從精靈圖 (Sprite Sheet) 剪下哪一個圖案
                if (board[i][j].isOpened) {
                    if (board[i][j].isMine) {
                        tileX = 4; tileY = 1; // 地雷圖案
                    }
                    else {
                        int n = board[i][j].neighborMines;
                        if (n == 0) { tileX = 3; tileY = 1; }                    // 空白凹陷方塊
                        else if (n >= 1 && n <= 5) { tileX = n - 1; tileY = 0; } // 數字 1~5
                        else if (n >= 6 && n <= 8) { tileX = n - 6; tileY = 1; } // 數字 6~8
                    }
                }
                else if (board[i][j].isFlagged) {
                    tileX = 0; tileY = 2; // 紅旗圖案
                }

                // 設定剪裁區域並指定螢幕繪製座標 (Y座標需加上 UI_HEIGHT 往下平移)
                sprite.setTextureRect(sf::IntRect(tileX * W, tileY * W, W, W));
                sprite.setPosition(j * W * SCALE, i * W * SCALE + UI_HEIGHT);
                window.draw(sprite); // 畫出方塊
            }
        }

        // 2. 繪製 UI 區塊的文字
        if (hasFont) {
            window.draw(timerText); // 畫計時器
            window.draw(helpText);  // 畫操作提示

            // 如果遊戲結束 (勝或敗)，在螢幕中央印出結果文字
            if (gameOver || gameWon) {
                statusText.setString(gameOver ? "GAME OVER" : "YOU WIN!");
                statusText.setFillColor(gameOver ? sf::Color::Red : sf::Color::Yellow);

                // 動態計算文字寬高，使其完美置中於地圖區域
                statusText.setPosition(
                    window.getSize().x / 2.0f - statusText.getGlobalBounds().width / 2.0f,
                    (window.getSize().y + UI_HEIGHT) / 2.0f - statusText.getGlobalBounds().height / 2.0f
                );
                window.draw(statusText);
            }
        }

        // 顯示渲染結果
        window.display();
    }
};

// ==========================================
// 3. 程式進入點 (Entry Point)
// ==========================================
int main() {
    Minesweeper game; // 實例化遊戲物件 (此時會執行建構子載入資源)
    game.run();       // 啟動遊戲主迴圈，直到視窗關閉才會 return
    return 0;         // 結束程式
}