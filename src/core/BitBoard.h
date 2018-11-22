// Copyright Chris Welty
//  All Rights Reserved
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

// board header file
#pragma once

#ifndef __CORE_BITBOARD_H
#define __CORE_BITBOARD_H

#if __GNUC__ >= 4 && defined(__x86_64__)
#include <xmmintrin.h>
#include <smmintrin.h>
#include <wmmintrin.h>
#elif defined(_WIN32)
#include <nmmintrin.h>
#endif

#include <cassert>
#include <stdio.h>
#include "../n64/utils.h"
class CMoves;

//! The bitboard representation of a position, containing two u64s: one for the mover and one for empty squares.
class CBitBoard {
public:
    u64 empty, mover;

    CBitBoard() : empty(0), mover(0) {}
    // Set to starting position
    void Initialize();
    void Initialize(const char* boardText, bool fBlackMove=true);

    // flipping
    void FlipHorizontal();
    void FlipVertical();
    void FlipDiagonal();
    CBitBoard Symmetry(int sym) const;
    CBitBoard MinimalReflection() const;

    void InvertColors();

    // impossible board
    void SetImpossible();
    bool IsImpossible() const;

    // validation
    bool IsValid() const { return !(empty&mover); }

    // I/O
    void Print(bool fBlackMove) const;// Print the board in human-readable format
    void FPrint(FILE* fp, bool blackMove) const;// Print the board in human-readable format
    bool Write(FILE* fp) const; // print in bitstream format
    bool Read(FILE* fp); // read in bitstream format; return true if successful
    char* GetSBoard(char sBoard[NN+1], bool fBlackMove) const;
    static constexpr __m128i xmm_key_1 { 0x7f9549b967963e86LL,  0x3ec4f67455147580LL};
    static constexpr __m128i xmm_key_2 { 0x216c272838d5e65bLL,   0x13e88ae85ba72bfLL};
    static constexpr __m128i xmm_key_3 { 0x125debcc5f19a4dbLL, -0x7d12beba70222040LL};
    static constexpr __m128i xmm_key_4 {-0x3adbf16345e7cc84LL,  0x75bc3d6b61ec11b1LL};
    static constexpr __m128i xmm_key_5 {-0x20bb65acc4a3bcdeLL, -0x4baa25957d8f982eLL};

    u64 Hash() const {
#if (__GNUC__ >= 4 && defined(__x86_64__)) || defined(_WIN32)
        uint64_t crc = _mm_crc32_u64(0xffffffff, empty);
        return (_mm_crc32_u64(crc, mover) * 0x10001ull);
#else
        u4 a, b, c, d;
        a = u4(empty);
        b = u4(empty >> 32);
        c = u4(mover);
        d = u4(mover >> 32);
        bobLookup(a, b, c, d);
        return d;
#endif
    }
    u32 XmmHash() const {
#if (__GNUC__ >= 4 && defined(__x86_64__)) || defined(_WIN32)
    	auto xmm_loaded = _mm_set_epi64x(empty, mover);
    	auto encoded = _mm_aesenclast_si128(_mm_aesenc_si128(_mm_aesenc_si128(_mm_aesenc_si128(_mm_aesenc_si128(xmm_loaded, xmm_key_1), xmm_key_2), xmm_key_3), xmm_key_4), xmm_key_5);
    	uint64_t hash =  _mm_extract_epi64(encoded, 0);
	return (hash >> 32 | hash) & 0xffffffff;
#else
        u4 a, b, c, d;
        a = u4(empty);
        b = u4(empty >> 32);
        c = u4(mover);
        d = u4(mover >> 32);
        bobLookup(a, b, c, d);
        return d;
#endif
    }
    // statistics

    int NEmpty() const { return bitCountInt(empty); }
    int NMover() const { return bitCountInt(mover); }
    void NDiscs(bool fBlackMove, int& nBlack, int& nWhite, int& nEmpty) const;
    int TerminalValue() const;

    // moving
    bool CalcMoves(CMoves& moves) const;
    int CalcMobility(u4& nMovesPlayer, u4& nMovesOpponent) const;
    int NMoverMobilities() const;
    bool operator==(const CBitBoard& b) const;
    bool operator!=(const CBitBoard& b) const;

    u64 getEnemy() const { return ~(u64(mover)|u64(empty)); }

protected:
    void FPrintHeader(FILE* fp) const;
};

bool operator<(const CBitBoard& a, const CBitBoard& b);

//! Make the bitboard an impossible position. This is used when clearing the hashtable.
inline void CBitBoard::SetImpossible() {
    mover=empty=~0;
    assert(IsImpossible());
}

//! Check whether the bitboard is impossible. Impossible positions are stored in unused
//! hashtable locations.
inline bool CBitBoard::IsImpossible() const {
    return (mover&empty)?true:false;
}

inline bool CBitBoard::operator==(const CBitBoard& b) const { return !((mover^b.mover)|(empty^b.empty));};
inline bool CBitBoard::operator!=(const CBitBoard& b) const { return mover!=b.mover || empty!=b.empty;};
inline void CBitBoard::FlipVertical() { empty=flipVertical(empty); mover=flipVertical(mover);}
inline void CBitBoard::FlipHorizontal() { empty=flipHorizontal(empty); mover=flipHorizontal(mover);}
inline void CBitBoard::FlipDiagonal() { empty=flipDiagonal(empty); mover=flipDiagonal(mover);}
inline void CBitBoard::InvertColors() { mover^=~empty; }

//! This is just a bitboard that's guaranteed to be a minimal reflection.
//!
//! The class won't completely guarantee that you haven't messed up, since you can alter the data
//! elements to make it a nonminimal bitboard. Don't do this.
class CMinimalReflection: public CBitBoard {
public:
    CMinimalReflection(const CBitBoard& bb) : CBitBoard(bb.MinimalReflection()) {};
};

void TestBitBoard();

#endif  // __CORE_BITBOARD_H
