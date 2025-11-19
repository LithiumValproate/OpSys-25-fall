#include <iomanip>
#include <iostream>
using namespace std;
using Byte_Count = long long;

enum class AllocAlgo {
    First_fit,
    Best_fit,
    Worst_fit,
    Next_fit
} currentAlgo = AllocAlgo::First_fit;

struct Block {
    int id;
    Byte_Count start;
    Byte_Count size;
    bool free;
    Block* next;
}* head = nullptr,* lastAllocPos = nullptr;

int nextId         = 1;
Byte_Count memSize = 1LL * 1024 * 1024;

void clearMemory(Block*);

void initMemory() {
    if (head) {
        clearMemory(head);
        head = nullptr;
    }

    cout << "Input Memory Size (default: " << memSize << ") :" << endl;
    Byte_Count sz;
    if (cin >> sz && sz > 0) memSize = sz;
    else {
        cout << "Invalid input" << endl;
        cin.clear();
        cin.ignore(1024LL * 1024LL, '\n');
        memSize = 1LL * 1024 * 1024;
    }

    head         = new Block;
    head->id     = 0;
    head->start  = 0;
    head->size   = memSize;
    head->free   = true;
    head->next   = nullptr;
    nextId       = 1;
    lastAllocPos = head;
    cout << "Memory Initialization Complete, Size = " << memSize << endl;
}

void allocFactory(Block*, Byte_Count);

int allocFirstFit(const Byte_Count reqSize) {
    Block* p = head;
    while (p) {
        if (p->free && p->size >= reqSize) {
            allocFactory(p, reqSize);
            cout << "Allocation completed. Block ID: " << nextId << endl;
            return nextId++;
        }
        p = p->next;
    }

    cout << "Allocation Error" << endl;
    return -1;
}

int allocBestFit(const Byte_Count reqSize) {
    Block* best = nullptr;
    Block* p    = head;
    while (p) {
        if (p->free && p->size >= reqSize) {
            if (!best || p->size < best->size) {
                best = p;
            }
        }
        p = p->next;
    }
    if (!best) {
        cout << "Allocation Error" << endl;
        return -1;
    }

    allocFactory(best, reqSize);
    cout << "Allocation completed. Block ID: " << nextId << endl;
    return nextId++;
}

int allocWorstFit(const Byte_Count reqSize) {
    Block* worst = nullptr;
    Block* p     = head;
    while (p) {
        if (p->free && p->size >= reqSize) {
            if (!worst || p->size > worst->size) {
                worst = p;
            }
        }
        p = p->next;
    }
    if (!worst) {
        cout << "Allocation Error" << endl;
        return -1;
    }

    allocFactory(worst, reqSize);
    cout << "Allocation completed. Block ID: " << nextId << endl;
    return nextId++;
}

int allocNextFit(const Byte_Count reqSize) {
    if (!head) {
        cout << "Allocation Error" << endl;
        return -1;
    }

    Block* start = head;
    if (lastAllocPos) {
        Block* q   = head;
        bool found = false;
        while (q) {
            if (q == lastAllocPos) {
                found = true;
                break;
            }
            q = q->next;
        }
        if (found) start = lastAllocPos;
    }
    Block* p = start;
    while (p) {
        if (p->free && p->size >= reqSize) {
            allocFactory(p, reqSize);
            lastAllocPos = p;
            cout << "Allocation completed. Block ID: " << nextId << endl;
            return nextId++;
        }
        p = p->next;
    }
    p = head;
    while (p && p != start) {
        if (p->free && p->size >= reqSize) {
            allocFactory(p, reqSize);
            lastAllocPos = p;
            cout << "Allocation completed. Block ID: " << nextId << endl;
            return nextId++;
        }
        p = p->next;
    }

    cout << "Allocation Error" << endl;
    return -1;
}

int allocateMemory(const Byte_Count reqSize) {
    if (reqSize <= 0) {
        cout << "Invalid Request Size" << endl;
        return -1;
    }
    switch (currentAlgo) {
        case AllocAlgo::First_fit: return allocFirstFit(reqSize);
        case AllocAlgo::Best_fit: return allocBestFit(reqSize);
        case AllocAlgo::Worst_fit: return allocWorstFit(reqSize);
        case AllocAlgo::Next_fit: return allocNextFit(reqSize);
    }
    return -1;
}

void allocFactory(Block* p, const Byte_Count reqSize) {
    if (p->size == reqSize) {
        p->free = false;
        p->id   = nextId;
    } else {
        auto* newBlock  = new Block;
        newBlock->id    = 0;
        newBlock->start = p->start + reqSize;
        newBlock->size  = p->size - reqSize;
        newBlock->free  = true;
        newBlock->next  = p->next;

        p->size = reqSize;
        p->free = false;
        p->id   = nextId;
        p->next = newBlock;
    }
}

void freeMemory(const int id) {
    if (id <= 0) {
        cout << "Invalid ID" << endl;
        return;
    }

    Block* p    = head;
    Block* prev = nullptr;
    while (p && p->id != id) {
        prev = p;
        p    = p->next;
    }
    if (!p) {
        cout << "ID Not Found" << endl;
        return;
    }

    if (p->free) {
        cout << p->id << " is already freed" << endl;
        return;
    }
    p->free = true;
    p->id   = 0;
    if (p->next && p->next->free) {
        Block* tmp = p->next;
        p->size += tmp->size;
        p->next = tmp->next;
        delete tmp;
    }
    if (prev && prev->free) {
        prev->size += p->size;
        prev->next = p->next;
        delete p;
        if (lastAllocPos == p) lastAllocPos = prev;
    }
    cout << "Block Freed" << endl;
}

void compactMemory() {
    if (!head) {
        cout << "Memory Uninitialized" << endl;
        return;
    }

    Block* p       = head;
    Block* newHead = nullptr;
    Block* tail    = nullptr;
    auto curr      = static_cast<Byte_Count>(0);
    while (p) {
        if (!p->free) {
            auto* b  = new Block;
            b->id    = p->id;
            b->size  = p->size;
            b->free  = false;
            b->start = curr;
            curr += b->size;
            b->next = nullptr;

            if (!newHead) newHead = tail = b;
            else {
                tail->next = b;
                tail       = b;
            }
        }
        p = p->next;
    }
    if (curr < memSize) {
        auto* freeBlock  = new Block;
        freeBlock->id    = 0;
        freeBlock->start = curr;
        freeBlock->size  = memSize - curr;
        freeBlock->free  = true;
        freeBlock->next  = nullptr;

        if (!newHead) newHead = freeBlock;
        else tail->next       = freeBlock;
    }
    clearMemory(head);
    head         = newHead;
    lastAllocPos = head;
    cout << "Memory Compacted" << endl;
}

void clearMemory(Block* h) {
    Block* p = h;
    while (p) {
        Block* tmp = p->next;
        delete p;
        p = tmp;
    }
}

void showMemory() {
    if (!head) {
        cout << "Memory Uninitialized" << endl;
        return;
    }

    cout << "\n===== Current Memory State =====\n";
    cout << "Algorithm: ";
    switch (currentAlgo) {
        case AllocAlgo::First_fit: cout << "First Fit";
            break;
        case AllocAlgo::Best_fit: cout << "Best Fit";
            break;
        case AllocAlgo::Worst_fit: cout << "Worst Fit";
            break;
        case AllocAlgo::Next_fit: cout << "Next Fit";
            break;
    }
    cout << "\nTotal Memory Size: " << memSize << "\n\n";

    cout << left
            << setw(10) << "ID"
            << setw(15) << "Start"
            << setw(15) << "End"
            << setw(15) << "Size"
            << setw(10) << "State"
            << "\n";

    cout << string(65, '-') << "\n";

    Block* p = head;
    while (p) {
        cout << left
                << setw(10) << p->id
                << setw(15) << p->start
                << setw(15) << (p->start + p->size)
                << setw(15) << p->size
                << setw(10) << (p->free ? "FREE" : "USED")
                << "\n";
        p = p->next;
    }

    cout << string(65, '=') << "\n\n";
}

#include "test.hpp"

int main() {
    int choice;
    Byte_Count req;
    int id;

    while (true) {
        cout << "\n====== Dynamic Partition Allocation ======\n";
        cout << "1. Initialize Memory\n";
        cout << "2. Allocate Memory\n";
        cout << "3. Free Memory\n";
        cout << "4. Compact Memory\n";
        cout << "5. Show Memory State\n";
        cout << "6. Select Allocation Algorithm\n";
        cout << "7. Run Test Script (from tests.hpp)\n";
        cout << "0. Exit\n";
        cout << "==========================================\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1024, '\n');
            continue;
        }

        switch (choice) {
            case 1:
                initMemory();
                break;
            case 2:
                cout << "Enter size to allocate: ";
                cin >> req;
                allocateMemory(req);
                break;
            case 3:
                cout << "Enter block ID to free: ";
                cin >> id;
                freeMemory(id);
                break;
            case 4:
                compactMemory();
                break;
            case 5:
                showMemory();
                break;
            case 6: {
                cout << "Select algorithm:\n";
                cout << "1. First Fit\n";
                cout << "2. Best Fit\n";
                cout << "3. Worst Fit\n";
                cout << "4. Next Fit\n";
                cout << "Enter option: ";
                int algo;
                cin >> algo;
                switch (algo) {
                    case 1: currentAlgo = AllocAlgo::First_fit;
                        break;
                    case 2: currentAlgo = AllocAlgo::Best_fit;
                        break;
                    case 3: currentAlgo = AllocAlgo::Worst_fit;
                        break;
                    case 4: currentAlgo = AllocAlgo::Next_fit;
                        break;
                    default: cout << "Invalid selection\n";
                        break;
                }
                break;
            }
            case 7:
                runTests(); // tests.hpp 中提供
                break;
            case 0:
                cout << "Exiting...\n";
                return 0;
            default:
                cout << "Invalid choice\n";
                break;
        }
    }
}
