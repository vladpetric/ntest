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

// #define USE_GOOGLE_SPARSE_HASH

#ifdef USE_GOOGLE_SPARSE_HASH
#include <sparsehash/sparse_hash_set>
#endif

#include "core/BitBoard.h"
#include "core/Moves.h"
#include "Evaluator.h"
#include "Search.h"
#include "pattern/FastFlip.h"
#include "n64/flips.h"

using namespace std;

struct CBitBoardHash {
    u64 operator()(const CBitBoard& bb) const {
        return bb.Hash();
    }
};

#ifdef USE_GOOGLE_SPARSE_HASH
typedef google::sparse_hash_set<CBitBoard, CBitBoardHash> PlySet;
#else
typedef unordered_set<CBitBoard, CBitBoardHash> PlySet;
#endif

void AllPositions() {
    CBitBoard bb;
    bb.Initialize();
    PlySet *current = new PlySet;
    current->insert(bb);
    PlySet *next = new PlySet;
    bool black_move = true;
    unsigned step;
    for (step = 0; step < 12; ++step, black_move = !black_move) {
        cout << "Move: " << step << " ply count " << current->size() << endl;
        for (CBitBoard cbb: *current) {
            u64 move_bits = mobility(cbb.mover, cbb.getEnemy());
            if (move_bits) {
                CMoves moves;
                moves.Set(move_bits);
                CMove move;
                while (moves.GetNext(move)) {
                    CBitBoard nbb = cbb;
                    const int sq = move.Square();
                    const uint64_t flip = flips(sq, nbb.mover, nbb.getEnemy());
                    const uint64_t square_mask = mask(sq);
                    nbb.mover ^= (flip | square_mask);
                    nbb.empty ^= square_mask;
                    nbb.InvertColors();
                    next->insert(nbb.MinimalReflection());
                }
            } else {
                CBitBoard nbb = cbb;
                u64 enemy_move_bits = mobility(nbb.getEnemy(), nbb.mover);
                if (enemy_move_bits) {
                    nbb.InvertColors();
                    next->insert(nbb.MinimalReflection());
                }
            }
        }
        PlySet *t = current;
        current = next;
        next = t;
        next->clear();
    }
    cout << "Move: " << step << " ply count " << current->size() << endl;
}

bool HasInput() { return false; }

int main(int argc, char **argv) {
    InitFastFlip();
    initFlips();
    InitConfigToPotMob();

    AllPositions();    
}
