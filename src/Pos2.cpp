// Copyright Chris Welty
//  All Rights Reserved
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

#include <iomanip>
#include <cassert>
#include <ctype.h>
#include <stdio.h>
#include <fstream>
#include <math.h>
#include "n64/flips.h"
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
    u64 flip = flips(square, m_bb.mover, ~(m_bb.mover | m_bb.empty)) | mask(square);
    assert ((m_stable & flip) == 0);
    m_bb.empty ^= mask(square);
    m_bb.mover ^= flip;

    /*
    if (flip & m_stable_trigger) {
      auto opponent = ~(m_bb.mover | m_bb.empty);
      
      m_stable = stable_discs(m_bb.mover, opponent, m_bb.empty, m_stable);
      m_stable_trigger = stable_next_mask(m_stable, ~m_bb.empty);
      
      m_stable_mover = bitCount(m_stable & m_bb.mover);
      m_stable_opponent = bitCount(m_stable & opponent);
    } */
    if (m_stable) {
        auto stable_swap = m_stable_mover;
        m_stable_mover = m_stable_opponent;
        m_stable_opponent = stable_swap;
    }

    m_bb.InvertColors();
 
    m_fBlackMove=!m_fBlackMove;
    nBBFlips++;
}

///////////////////////////////////////////////////////////////////////////////
// Debug and print routines
///////////////////////////////////////////////////////////////////////////////

// debug stuff
void FPrintHeader(std::stringstream &ss);
void FPrintRow(std::stringstream &ss, int row, int pattern, int nColors[3]);
void PrintSquareDirectionToPattern();
void PrintSquareDirectionToValue();

void Pos2::Print() const {
    FPrint(stdout);
}

void Pos2::FPrint(FILE* fp) const {
    int row, nColors[3];
    std::stringstream ss;

    FPrintHeader(ss);

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

        FPrintRow(ss, row, rowPattern, nColors);
    }

    FPrintHeader(ss);

    ss << "\nBlack: " << nColors[2] << "  White: " << nColors[0] << "  Empty: " << nColors[1] << "\n\n";
    ss << (m_fBlackMove ? "Black" : "White") << " to move\n";

    std::string output = ss.str();
    fwrite(output.c_str(), 1, output.size(), fp);
}

void Pos2::PrintStable() const {
    FPrintStable(stdout);
}
void Pos2::PrintStableNext() const {
    FPrintStableNext(stdout);
}
void Pos2::FPrintStable(FILE* fp) const {
    int row, nColors[3];
    std::stringstream ss;

    FPrintHeader(ss);

    nColors[0]=nColors[1]=nColors[2]=0;
    for (row=0; row<N; row++) {
        uint64_t bitrow = (m_stable >> (row * 8))& 0xff;
        // print row number
        ss << std::setw(2) << row + 1;

        // break row apart into values and print value
        for (int col=0; col<N; col++) {
            auto item = bitrow & 1;
            bitrow >>= 1;
            ss << " " << (item ? 'X' : '-');
        }

        //print row number at right of row
        ss << " " << std::setw(2) << row + 1 << "\n";
    }

    FPrintHeader(ss);

    ss << "\nBlack: " << nColors[2] << "  White: " << nColors[0] << "  Empty: " << nColors[1] << "\n\n";
    ss << (m_fBlackMove ? "Black" : "White") << " to move\n";

    std::string output = ss.str();
    fwrite(output.c_str(), 1, output.size(), fp);
}
void Pos2::FPrintStableNext(FILE* fp) const {
    int row, nColors[3];
    std::stringstream ss;

    FPrintHeader(ss);

    nColors[0]=nColors[1]=nColors[2]=0;
    for (row=0; row<N; row++) {
        uint64_t bitrow = (m_stable_trigger >> (row * 8))& 0xff;
        // print row number
        ss << std::setw(2) << row + 1;

        // break row apart into values and accumulate value
        for (int col=0; col<N; col++) {
            auto item = bitrow & 1;
            bitrow >>= 1;
            ss << " " << (item ? 'X' : '-');
        }

        //print row number at right of row
        ss << " " << std::setw(2) << row + 1 << "\n";
    }

    FPrintHeader(ss);

    ss << "\nBlack: " << nColors[2] << "  White: " << nColors[0] << "  Empty: " << nColors[1] << "\n\n";
    ss << (m_fBlackMove ? "Black" : "White") << " to move\n";

    std::string output = ss.str();
    fwrite(output.c_str(), 1, output.size(), fp);
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

void FPrintHeader(std::stringstream &ss) {
    int col;
    ss << "  ";
    for (col=0; col<N; col++)
        ss << " " << static_cast<char>('A' + col);
    ss << "\n";
}

void FPrintRow(std::stringstream &ss, int row, int config, int nColors[3]) {
    int col, item;

    // print row number
    ss << std::setw(2) << row + 1;

    // break row apart into values and print value
    for (col=0; col<N; col++) {
        item = config%3;
        config=(config-item)/3;
        ss << " " << ValueToText(item);
        nColors[item]++;
    }

    //print row number at right of row
    ss << " " << std::setw(2) << row + 1 << "\n";
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
