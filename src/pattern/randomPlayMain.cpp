#include "../n64/flips.h"
#include "randomPlayTest.h"

int HasInput() { return false; }
int main() {
    initFlips();
    TestRandomGames(100000);
}

