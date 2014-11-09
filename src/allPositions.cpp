// Copyright Vlad Petric
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/BitBoard.h"
#include "core/Moves.h"
#include "Evaluator.h"
#include "Search.h"
#include "pattern/FastFlip.h"

using namespace std;

struct CBitBoardHash {
    u64 operator()(const CBitBoard& bb) const {
        return bb.Hash();
    }
};

struct CBitBoardComp {
    u64 operator()(const CBitBoard& a, const CBitBoard& b) const {
        return std::make_pair(a.mover, a.empty) < std::make_pair(b.mover, b.empty);
    }
};

typedef unordered_set<CBitBoard, CBitBoardHash> PlySet;
//typedef set<CBitBoard, CBitBoardComp> PlySet;

void AllPositions() {
    CBitBoard bb;
    bb.Initialize();
    PlySet current;
    current.insert(bb);
    PlySet next;
    bool black_move = true;
    unsigned step;
    for (step = 0; step < 11; ++step, black_move = !black_move) {
        cout << "Move: " << step << " ply count " << current.size() << endl;
        for (CBitBoard cbb: current) {
            //cbb.FPrint(stdout, black_move);
            //fprintf(stdout, "\n");
            u64 move_bits = mobility(cbb.mover, cbb.getEnemy());
            if (move_bits) {
                CMoves moves;
                moves.Set(move_bits);
                while (moves.HasMoves()) {
                    CBitBoard nbb = cbb;
                    CMove move;
                    moves.GetNext(move);
                    const int sq = move.Square();
                    const uint64_t flip = flips(sq, nbb.mover, nbb.getEnemy());
                    //fprintf(stdout, "Flip Mask: %llx\n", flip);
                    const uint64_t square_mask = mask(sq);
                    nbb.mover ^= (flip | square_mask);
                    nbb.empty ^= square_mask;
                    nbb.InvertColors();
                    next.insert(nbb.MinimalReflection());
                }
            } else {
                u64 enemy_move_bits = mobility(cbb.getEnemy(), cbb.mover);
                if (enemy_move_bits) {
                    std::cout << "Pass encountered" << std::endl;
                    cbb.InvertColors();
                    next.insert(cbb.MinimalReflection());
                } else {
                    std::cout << "End of game encountered" << std::endl;
                }
            }
        }
        std::swap(current, next);
        next.clear();
    }
    cout << "Move: " << step << " ply count " << current.size() << endl;
}

int main(int argc, char **argv) {
    InitFastFlip();
    initFlips();
    InitConfigToPotMob();

    AllPositions();    
}
