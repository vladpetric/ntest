// Copyright Chris Welty
// All Rights Reserved
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

#include <iomanip>
#include <cassert>
#include <ctype.h>
#include <stdio.h>
#include <fstream>
#include <math.h>
#include "n64/types.h"
#include "n64/test.h"
#include "n64/utils.h"
#include "core/BitBoard.h"
#include "core/QPosition.h"
#include "core/Moves.h"
#include "core/options.h"
#include "core/NodeStats.h"
#include "pattern/Patterns.h"
#include "pattern/FastFlip.h"

#include "Pos2.h"
#include "Evaluator.h"
#include "options.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// position variables
///////////////////////////////////////////////////////////////////////////////

// initialize position.
void Pos2::InitializeToStartPosition() {
    Initialize("...........................O*......*O...........................", true);
}

// inputs:
//    text representing the board
void Pos2::Initialize(const char* sBoard, bool fBlackMove) {
    m_fBlackMove=fBlackMove;
    m_bb.Initialize(sBoard, m_fBlackMove);
}

void Pos2::Initialize(const CBitBoard& m_bb, bool m_fBlackMove) {
    char sBoard[NN+1];

    m_bb.GetSBoard(sBoard, m_fBlackMove);
    Initialize(sBoard, m_fBlackMove);
}


///////////////////////////////////////////////////////////////////////////////
// Move routines
///////////////////////////////////////////////////////////////////////////////

void Pos2::MakeMoveBB(int square) {
    int nflipped;
    u64 flip = flips(square, m_bb.mover, ~(m_bb.mover | m_bb.empty)) | mask(square);
    assert ((m_stable & flip) == 0);
    m_bb.empty ^= mask(square);
    m_bb.mover ^= flip;
    if (flip & m_stable_trigger) {
      //std::cout << "Enhancing stable region" << std::endl;
      m_stable = stable_discs(m_bb.mover, ~(m_bb.mover | m_bb.empty), m_bb.empty,
                              m_stable);
      assert ((m_stable & ~m_bb.empty) == m_stable);
      m_stable_trigger = stable_next_mask(m_stable, ~m_bb.empty);
    }
    m_bb.InvertColors();
 
    m_fBlackMove=!m_fBlackMove;
    nBBFlips++;
}

///////////////////////////////////////////////////////////////////////////////
// Debug and print routines
///////////////////////////////////////////////////////////////////////////////

// debug stuff
void FPrintHeader(FILE* fp);
void FPrintRow(FILE* fp, int row, int pattern, int nColors[3]);
void PrintSquareDirectionToPattern();
void PrintSquareDirectionToValue();

void Pos2::Print() const {
    FPrint(stdout);
}

void Pos2::FPrint(FILE* fp) const {
    int row, nColors[3];

    FPrintHeader(fp);

    uint64_t black;
    if (this->BlackMove()) {
        black = m_bb.mover;
    } else {
        black = ~(m_bb.mover | m_bb.empty);
    }


    nColors[0]=nColors[1]=nColors[2]=0;
    for (row=0; row<N; row++) {
        int16_t rowPattern =
                base2ToBase3Table[(m_bb.empty >> (row * 8))& 0xff] +
                base2ToBase3Table[(black >> (row * 8))& 0xff] * 2;

        FPrintRow(fp, row, rowPattern, nColors);
    }

    FPrintHeader(fp);

    fprintf(fp, "\nBlack: %d  White: %d  Empty: %d\n\n",nColors[2],nColors[0],nColors[1]);
    fprintf(fp, "%s to move\n",m_fBlackMove?"Black":"White");
}
void Pos2::PrintStable() const {
    FPrintStable(stdout);
}
void Pos2::PrintStableNext() const {
    FPrintStableNext(stdout);
}
void Pos2::FPrintStable(FILE* fp) const {
    int row, nColors[3];

    FPrintHeader(fp);

    uint64_t black;
    if (this->BlackMove()) {
        black = m_bb.mover;
    } else {
        black = ~(m_bb.mover | m_bb.empty);
    }


    nColors[0]=nColors[1]=nColors[2]=0;
    for (row=0; row<N; row++) {
        uint64_t bitrow = (m_stable >> (row * 8))& 0xff;
        // print row number
        fprintf(fp, "%-2d", row+1);

        // break row apart into values and print value
        for (int col=0; col<N; col++) {
            auto item = bitrow & 1;
            bitrow >>= 1;
            fprintf(fp, "%c ", item? 'X' : '-');
        }

        //print row number at right of row
        fprintf(fp, "%2d\n", row+1);
    }

    FPrintHeader(fp);

    fprintf(fp, "\nBlack: %d  White: %d  Empty: %d\n\n",nColors[2],nColors[0],nColors[1]);
    fprintf(fp, "%s to move\n",m_fBlackMove?"Black":"White");
}
void Pos2::FPrintStableNext(FILE* fp) const {
    int row, nColors[3];

    FPrintHeader(fp);

    uint64_t black;
    if (this->BlackMove()) {
        black = m_bb.mover;
    } else {
        black = ~(m_bb.mover | m_bb.empty);
    }


    nColors[0]=nColors[1]=nColors[2]=0;
    for (row=0; row<N; row++) {
        uint64_t bitrow = (m_stable_trigger >> (row * 8))& 0xff;
        // print row number
        fprintf(fp, "%-2d", row+1);

        // break row apart into values and print value
        for (int col=0; col<N; col++) {
            auto item = bitrow & 1;
            bitrow >>= 1;
            fprintf(fp, "%c ", item? 'X' : '-');
        }

        //print row number at right of row
        fprintf(fp, "%2d\n", row+1);
    }

    FPrintHeader(fp);

    fprintf(fp, "\nBlack: %d  White: %d  Empty: %d\n\n",nColors[2],nColors[0],nColors[1]);
    fprintf(fp, "%s to move\n",m_fBlackMove?"Black":"White");
}

char* Pos2::GetText(char* sBoard) const {
    int row,col,sq,item;

    uint64_t black;
    if (this->BlackMove()) {
        black = m_bb.mover;
    } else {
        black = ~(m_bb.mover | m_bb.empty);
    }

    for (sq=row=0; row<N; row++) {
        int16_t pattern =
                base2ToBase3Table[(m_bb.empty >> (row * 8))& 0xff] +
                base2ToBase3Table[(black >> (row * 8))& 0xff] * 2;
        for (col=0; col<N; col++) {
            item=pattern%3;
            pattern=(pattern-item)/3;
            sBoard[sq++]=ValueToText(item);
        }
    }

    sBoard[sq]=0;
    return sBoard;
}


///////////////////////////////////////////////////////////////////
// Debug and print functions - protected
///////////////////////////////////////////////////////////////////

void FPrintHeader(FILE* fp) {
    int col;

    fprintf(fp, "  ");
    for (col=0; col<N; col++)
        fprintf(fp, "%c ",col+'A');
    fprintf(fp, "\n");
}

void FPrintRow(FILE* fp, int row, int config, int nColors[3]) {
    int col, item;

    // print row number
    fprintf(fp, "%-2d", row+1);

    // break row apart into values and print value
    for (col=0; col<N; col++) {
        item=config%3;
        config=(config-item)/3;
        fprintf(fp, "%c ",ValueToText(item));
        nColors[item]++;
    }

    //print row number at right of row
    fprintf(fp, "%2d\n", row+1);
}

// CalcMovesAndPass - calc moves. decide if the mover needs to pass, and pass if he does
// inputs:
//    none
// outputs:
//    moves - the moves available
//    pass -    0 if the next player can move
//            1 if the next player must pass but game is not over
//            2 if both players must pass and game is over
//    If pass>0 the position is left with one move passed
int Pos2::CalcMovesAndPassBB(CMoves& moves) {
    if (CalcMoves(moves))
        return 0;
    else {
        PassBB();
        if (CalcMoves(moves))
            return 1;
        else
            return 2;
    }
}

int Pos2::CalcMovesAndPassBB(CMoves& moves, const CMoves& submoves) {
    if (submoves.HasMoves()) {
        moves=submoves;
        return 0;
    }
    else {
        PassBB();
        if (CalcMoves(moves))
            return 1;
        else
            return 2;
    }
}
