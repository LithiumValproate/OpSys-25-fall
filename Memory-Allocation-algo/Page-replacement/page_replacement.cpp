#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
using std::pair;
using std::vector;
using Access_Res = pair<bool, int>;

enum class ReplaceAlgo {
    Fifo_algo,
    Opt_algo,
    Lru_algo,
};

struct Frame {
    int page   = 0;
    bool valid = false;
};

class AlgoState {
public:
    virtual ~AlgoState() = default;
    virtual Access_Res access(int step, int page, vector<Frame>& frames,
                              const vector<int>& ref) = 0;
};

class FifoState final : public AlgoState {
    int nextIndex_;

public:
    explicit FifoState(int /*frameCount*/) : nextIndex_(0) {}

    Access_Res access(int /*step*/, int page, vector<Frame>& frames,
                      const vector<int>& ref) override {
        (void) ref; // FIFO does not need the full reference string

        for (const auto& f : frames) {
            if (f.valid && f.page == page) {
                return {true, -1};
            }
        }

        for (size_t i = 0; i < frames.size(); ++i) {
            if (!frames[i].valid) {
                frames[i].page  = page;
                frames[i].valid = true;
                return {false, static_cast<int>(i)};
            }
        }

        int victim           = nextIndex_;
        nextIndex_           = (nextIndex_ + 1) % static_cast<int>(frames.size());
        frames[victim].page  = page;
        frames[victim].valid = true;
        return {false, victim};
    }
};

class LruState final : public AlgoState {
    vector<int> lastUsed_;

public:
    explicit LruState(int frameCount) : lastUsed_(frameCount, -1) {}

    Access_Res access(int step, int page, vector<Frame>& frames,
                      const vector<int>& ref) override {
        (void) ref;

        for (size_t i = 0; i < frames.size(); ++i) {
            if (frames[i].valid && frames[i].page == page) {
                lastUsed_[i] = step;
                return {true, -1};
            }
        }

        for (size_t i = 0; i < frames.size(); ++i) {
            if (!frames[i].valid) {
                frames[i].page  = page;
                frames[i].valid = true;
                lastUsed_[i]    = step;
                return {false, static_cast<int>(i)};
            }
        }

        int victim = 0;
        int oldest = lastUsed_[0];
        for (size_t i = 1; i < frames.size(); ++i) {
            if (lastUsed_[i] < oldest) {
                oldest = lastUsed_[i];
                victim = static_cast<int>(i);
            }
        }

        frames[victim].page  = page;
        frames[victim].valid = true;
        lastUsed_[victim]    = step;
        return {false, victim};
    }
};

class OptState final : public AlgoState {
public:
    explicit OptState(int /*frameCount*/) {}

    Access_Res access(int step, int page, vector<Frame>& frames,
                      const vector<int>& ref) override {
        for (const auto& f : frames) {
            if (f.valid && f.page == page) {
                return {true, -1};
            }
        }

        for (size_t i = 0; i < frames.size(); ++i) {
            if (!frames[i].valid) {
                frames[i].page  = page;
                frames[i].valid = true;
                return {false, static_cast<int>(i)};
            }
        }

        int victim          = -1;
        int farthestNextUse = -1;

        for (size_t i = 0; i < frames.size(); ++i) {
            int nextUse = -1;
            for (size_t j = step + 1; j < ref.size(); ++j) {
                if (ref[j] == frames[i].page) {
                    nextUse = static_cast<int>(j);
                    break;
                }
            }

            if (nextUse == -1) {
                victim = static_cast<int>(i);
                break;
            }

            if (nextUse > farthestNextUse) {
                farthestNextUse = nextUse;
                victim          = static_cast<int>(i);
            }
        }

        if (victim == -1) {
            victim = 0;
        }

        frames[victim].page  = page;
        frames[victim].valid = true;
        return {false, victim};
    }
};

struct StepResult {
    int step;
    int page;
    bool hit;
    int victim;
    vector<Frame> frames;
};

std::unique_ptr<AlgoState> newAlgoState(ReplaceAlgo algo, int frameCount) {
    switch (algo) {
        case ReplaceAlgo::Fifo_algo:
            return std::make_unique<FifoState>(frameCount);
        case ReplaceAlgo::Opt_algo:
            return std::make_unique<OptState>(frameCount);
        case ReplaceAlgo::Lru_algo:
            return std::make_unique<LruState>(frameCount);
        default:
            return std::make_unique<FifoState>(frameCount);
    }
}

vector<StepResult> simulate(ReplaceAlgo algo, int frameCount, const vector<int>& ref) {
    vector<Frame> frames(frameCount);
    auto state = newAlgoState(algo, frameCount);
    vector<StepResult> results;
    results.reserve(ref.size());

    for (size_t step = 0; step < ref.size(); ++step) {
        auto [hit, victim] = state->access(static_cast<int>(step), ref[step], frames, ref);
        results.push_back(StepResult{
                          static_cast<int>(step),
                          ref[step],
                          hit,
                          victim,
                          frames,
                          });
    }

    return results;
}

std::string algoName(ReplaceAlgo algo) {
    switch (algo) {
        case ReplaceAlgo::Fifo_algo: return "FIFO";
        case ReplaceAlgo::Opt_algo: return "OPT";
        case ReplaceAlgo::Lru_algo: return "LRU";
    }
    return "Unknown";
}

std::string frameSnapshot(const vector<Frame>& frames) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < frames.size(); ++i) {
        if (i) oss << " | ";
        if (frames[i].valid) oss << frames[i].page;
        else oss << "-";
    }
    oss << "]";
    return oss.str();
}

void printResults(const vector<StepResult>& results) {
    int hits   = 0;
    int faults = 0;

    std::cout << std::left
              << std::setw(6) << "Step"
              << std::setw(8) << "Page"
              << std::setw(8) << "Hit?"
              << std::setw(10) << "Victim"
              << "Frames\n";
    std::cout << std::string(60, '-') << "\n";

    for (const auto& res : results) {
        if (res.hit) ++hits;
        else ++faults;
        std::cout << std::left
                  << std::setw(6) << res.step
                  << std::setw(8) << res.page
                  << std::setw(8) << (res.hit ? "Yes" : "No")
                  << std::setw(10) << (res.victim >= 0 ? std::to_string(res.victim) : "-")
                  << frameSnapshot(res.frames) << "\n";
    }

    std::cout << "\nHits: " << hits << ", Faults: " << faults
              << ", Hit Ratio: " << (results.empty() ? 0.0 : static_cast<double>(hits) / results.size())
              << "\n";
}

ReplaceAlgo selectAlgo(int choice) {
    switch (choice) {
        case 1: return ReplaceAlgo::Fifo_algo;
        case 2: return ReplaceAlgo::Opt_algo;
        case 3: return ReplaceAlgo::Lru_algo;
        default: return ReplaceAlgo::Fifo_algo;
    }
}

int main() {
    std::cout << "==== Page Replacement Simulator ====\n";
    std::cout << "Algorithms: 1) FIFO  2) OPT  3) LRU\n";
    std::cout << "Enter 0 as algorithm choice to exit.\n\n";

    while (true) {
        std::cout << "Select algorithm (0 to exit): ";
        int algoChoice;
        if (!(std::cin >> algoChoice)) {
            std::cin.clear();
            std::cin.ignore(1024, '\n');
            continue;
        }
        if (algoChoice == 0) {
            std::cout << "Exiting...\n";
            return 0;
        }

        std::cout << "Enter frame count: ";
        int frames;
        if (!(std::cin >> frames) || frames <= 0) {
            std::cout << "Invalid frame count.\n";
            std::cin.clear();
            std::cin.ignore(1024, '\n');
            continue;
        }

        std::cout << "Enter reference string (space separated integers):\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);
        vector<int> refs;
        int value;
        while (iss >> value) {
            refs.push_back(value);
        }

        if (refs.empty()) {
            std::cout << "Reference string cannot be empty.\n";
            continue;
        }

        auto algo = selectAlgo(algoChoice);
        std::cout << "\nRunning " << algoName(algo) << " with "
                  << frames << " frames on " << refs.size() << " references.\n\n";

        auto results = simulate(algo, frames, refs);
        printResults(results);
        std::cout << "\n";
    }
}
