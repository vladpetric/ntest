#include "../n64/flips.h"
#include "randomPlayTest.h"

int main() {
    initFlips();
    TestRandomGames(100000);
}

