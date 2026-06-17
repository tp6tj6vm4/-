#define NOMINMAX
#include "GameEngine.h"

int main() {
    GameEngine engine; // 建立控制引擎（自動串聯 Model 與 View 資源）
    engine.run();      // 啟動主迴圈
    return 0;
}