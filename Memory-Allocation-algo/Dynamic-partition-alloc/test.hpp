#ifndef TESTS_HPP
#define TESTS_HPP

#include <iostream>
using std::cout;

extern void initMemory();
extern int allocateMemory(long long);
extern void freeMemory(int);
extern void compactMemory();
extern void showMemory();
extern AllocAlgo currentAlgo;

inline void runTests() {
    cout << "\n===== Running Test Script =====\n";

    // 1. 初始化 1MB 内存
    cout << "\n[TEST] initMemory()\n";
    initMemory();
    showMemory();

    // 2. First Fit 测试
    cout << "\n[TEST] Allocate 100, 200, 300\n";
    allocateMemory(100);  // id 1
    allocateMemory(200);  // id 2
    allocateMemory(300);  // id 3
    showMemory();

    // 3. Free block 2
    cout << "\n[TEST] Free block id=2\n";
    freeMemory(2);
    showMemory();

    // 4. Next Fit 测试
    cout << "\n[TEST] Switch to Next Fit, allocate 150\n";
    currentAlgo = AllocAlgo::Next_fit;
    allocateMemory(150);   // id 4
    showMemory();

    // 5. Best Fit 测试
    cout << "\n[TEST] Switch to Best Fit, allocate 80\n";
    currentAlgo = AllocAlgo::Best_fit;
    allocateMemory(80);    // id 5
    showMemory();

    // 6. Worst Fit 测试
    cout << "\n[TEST] Switch to Worst Fit, allocate 50\n";
    currentAlgo = AllocAlgo::Worst_fit;
    allocateMemory(50);    // id 6
    showMemory();

    // 7. 紧缩
    cout << "\n[TEST] Compact Memory\n";
    compactMemory();
    showMemory();

    cout << "\n===== Test Script Finished =====\n";
}

#endif
