#include <cstdlib>
#include <cstring>
#include "position.h"
#include "uci.h"

int main(int argc, char** argv) {
    initEngine();
    if (argc > 1 && !std::strcmp(argv[1], "bench")) { runBench(argc > 2 ? std::atoi(argv[2]) : 6); return 0; }
    uciLoop();
    return 0;
}
