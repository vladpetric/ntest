// Copyright Chris Welty, Vlad Petric
// All Rights Reserved
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

// Evaluator source code
#include <x86intrin.h>
#include <sstream>
#include "core/QPosition.h"
#include "n64/bitextractor.h"
#include "n64/extract_and_convert.hpp"

#include "Evaluator.h"

using namespace std;

//////////////////////////////////////////////////
// Evaluator base class
//////////////////////////////////////////////////

#if defined(__clang__)
#define INLINE_HINT inline
typedef unsigned long TConfig;
#elif __GNUC__ >= 4
#define INLINE_HINT inline __attribute__((always_inline)) 
typedef unsigned long TConfig;
#elif defined(WIN32)
#define INLINE_HINT __forceinline
typedef uint64_t TConfig;
#else
#define INLINE_HINT inline
typedef unsigned long TConfig;
#endif

class CEvaluatorList: public std::map<CEvaluatorInfo, CEvaluator*> {
public:
    ~CEvaluatorList();
};

static CEvaluatorList evaluatorList;

CEvaluatorList::~CEvaluatorList() {
    for (iterator it=begin(); it!=end(); it++) {
        delete it->second;
    }
}

CEvaluator* CEvaluator::FindEvaluator(char evaluatorType, char coeffSet) {
    CEvaluatorInfo evaluatorInfo;
    CEvaluator* result;
    map<CEvaluatorInfo, CEvaluator*>::iterator ptr;

    evaluatorInfo.evaluatorType=evaluatorType;
    evaluatorInfo.coeffSet=coeffSet;

    ptr=evaluatorList.find(evaluatorInfo);
    if (ptr==evaluatorList.end()) {
        switch(evaluatorType) {
        case 'J': {
            int nFiles= (coeffSet>='9')?10:6;
            result=new CEvaluator(FNBase(evaluatorType, coeffSet), nFiles);
            break;
                  }
        default:
            assert(0);
        }
        assert(result);
        evaluatorList[evaluatorInfo]=result;
    }
    else {
        result=(*ptr).second;
    }
    return result;
}

std::string CEvaluator::FNBase(char evaluatorType, char coeffSet) {
    std::ostringstream os;
    os << "coefficients/" << evaluatorType << coeffSet;
    return os.str();
}

//////////////////////////////////////////////////////
// Pattern J evaluator
//    Use 2x4, 2x5, edge+X patterns
//////////////////////////////////////////////////////

//! Conver the file to i2 format
//!
//! read in the file (float format) and write it out in i2 format.
//! reopen the file and read the iversion and fParams flags
static void ConvertFile(FILE*& fp, std::string fn, int& iVersion, u4& fParams) {
    // convert float coefficient file to int. Write new coefficients file to disk and reload.
    std::vector<i2> newCoeffs;
    float oldCoeff;
    while (fread(&oldCoeff, sizeof(oldCoeff), 1, fp)) {
        int coeff=int(oldCoeff*kStoneValue);
        if (coeff>0x3FFF)
            coeff=0x3FFF;
        if (coeff<-0x3FFF)
            coeff=-0x3FFF;
        newCoeffs.push_back(i2(coeff));
    }
    fclose(fp);
    fp=fopen(fn.c_str(), "wb");
    if (!fp)
        throw std::string("Can't open coefficient file for conversion: ")+fn;
    fParams=100;
    fwrite(&iVersion, sizeof(int), 1, fp);
    fwrite(&fParams, sizeof(int), 1, fp);
    fwrite(&newCoeffs[0], sizeof(u2), newCoeffs.size(), fp);
    fclose(fp);
    fp=fopen(fn.c_str(), "rb");
    if (!fp)
        throw std::string("Can't open coefficient file ")+fn;
    size_t result=fread(&iVersion, sizeof(int), 1, fp);
    assert(result==1);
    result=fread(&fParams, sizeof(int), 1, fp);
    assert(result==1);
}

//! Read in Evaluator coefficients from a coefficient file
//!
//! If the file's fParams is 14, coefficients are stored as floats and are in units of stones
//! If the file's fParams is 100, coefficients are stored as u2s and are in units of centistones.
//!
//! \throw string if error
CEvaluator::CEvaluator(const std::string& fnBase, int nFiles) {
    int map,  iFile, coeffStart, packedCoeff;
    int nIDs, nConfigs, id, config, cid;
    uint32_t configpm1, configpm2, mapsize;
    bool fHasPotMobs;
    float* rawCoeffs=0;    //!< for use with raw (float) coeffs
    i2* i2Coeffs=0;        //!< for use with converted (packed) coeffs
    TCoeff coeff;
    int iSubset, nSubsets, nEmpty;

    // some parameters are set based on the Evaluator version number
    int nSetWidth=60/nFiles;
    char cCoeffSet=fnBase.end()[-1];


    // read in sets
    nSets=0;
    for (iFile=0; iFile<nFiles; iFile++) {
        FILE* fp;
        std::string fn;
        int iVersion;

        // get file name
        std::ostringstream os;
        os << fnBase << char('a'+(iFile%nFiles)) << ".cof";
        fn=os.str();

        // open file
        fp=fopen(fn.c_str(),"rb");
        if (!fp)
            throw std::string("Can't open coefficient file ")+fn;

        // read in version and parameter info
        u4 fParams;
        assert(fread(&iVersion, sizeof(iVersion), 1, fp) == 1);
        assert(fread(&fParams, sizeof(fParams), 1, fp) == 1);
        if (iVersion==1 && fParams==14) {
            ConvertFile(fp, fn, iVersion, fParams);
        }
        if (iVersion!=1 || (fParams!=100)) {
            throw std::string("error reading from coefficients file ")+fnBase;
        }

        // calculate the number of subsets
        nSubsets=2;

        for (iSubset=0; iSubset<nSubsets; iSubset++) {
            // allocate memory for the black and white versions of the coefficients
            coeffs[nSets]=new TCoeff[nCoeffsJ];
            CHECKNEW(coeffs[nSets] != NULL);

            // put the coefficients in the proper place
            for (map=0; map<nMapsJ; map++) {

                // inital calculations
                nIDs=mapsJ[map].NIDs();
                nConfigs=mapsJ[map].NConfigs();
                mapsize=mapsJ[map].size;
                coeffStart=coeffStartsJ[map];

                // get raw coefficients from file
                i2Coeffs=new i2[nIDs];
                CHECKNEW(i2Coeffs!=NULL);
                if (fread(i2Coeffs,sizeof(u2),nIDs,fp)<size_t(nIDs))
                    throw std::string("error reading from coefficients file")+fnBase;

                // convert raw coefficients[id] to i2s[config]
                for (config=0; config<nConfigs; config++) {
                    id=mapsJ[map].ConfigToID(config);

                    coeff = i2Coeffs[id];

                    cid=config+coeffStart;

                    if (map==PARJ) {
                        // odd-even correction, only in Parity coefficient.
                        if (cCoeffSet>='A') {
                            if (iFile>=7)
                                coeff+=TCoeff(kStoneValue*.65);
                            else if (iFile==6)
                                coeff+=TCoeff(kStoneValue*.33);
                        }
                    }
                    
                    // coeff value has first 2 bytes=coeff, 3rd byte=potmob1, 4th byte=potmob2
                    if (map<M1J) {    // pattern maps
                        // restrict the coefficient to 2 bytes
                        if (coeff>0x3FFF)
                            packedCoeff=0x3FFF;
                        if (coeff<-0x3FFF)
                            packedCoeff=-0x3FFF;
                        else
                            packedCoeff=coeff;
                        
                        // get linear pot mob info
                        if (map<=D5J) {    // straight-line maps
                            fHasPotMobs=true;
                            configpm1=configToPotMob[0][mapsize][config];
                            configpm2=configToPotMob[1][mapsize][config];
                        }
                        
                        else if (map==C4J) {    // corner triangle maps
                            fHasPotMobs=true;
                            configpm1=configToPotMobTriangle[0][config];
                            configpm2=configToPotMobTriangle[1][config];
                        }

                        else {    // 2x4, 2x5, edge+2X
                            fHasPotMobs=false;
                        }

                        // pack coefficient and pot mob together
                        if (fHasPotMobs)
                            packedCoeff=(packedCoeff<<16) | (configpm1<<8) | (configpm2);                        

                        coeffs[nSets][cid]=packedCoeff;
                    }
                    
                    else {        // non-pattern maps
                        coeffs[nSets][cid]=coeff;
                    }
                }

                delete[] rawCoeffs;
                delete[] i2Coeffs;
            }

            // fold 2x4 corners into 2x5 corners
            TCoeff* pcf2x4, *pcf2x5;
            TConfig c2x4;

            pcf2x4=&(coeffs[nSets][coeffStartsJ[C2x4J]]);
            pcf2x5=&(coeffs[nSets][coeffStartsJ[C2x5J]]);
            // fold coefficients in
            for (config=0; config<9*6561; config++) {
                c2x4=configs2x5To2x4[config];
                pcf2x5[config]+=pcf2x4[c2x4];
            }
            // zero out 2x4 coefficients
            for (config=0;config<6561; config++) {
                pcf2x4[config]=0;
            }

            // Set the pcoeffs array and the fParameters
            for (nEmpty=59-nSetWidth*iFile; nEmpty>=50-nSetWidth*iFile; nEmpty--) {
                // if this is a set of the wrong parity, do nothing
                if ((nEmpty&1)==iSubset)
                    continue;
                pcoeffs[nEmpty]=coeffs[nSets];
            }

            nSets++;
        }
        fclose(fp);
    }
}

CEvaluator::~CEvaluator() {
    int set;

    // delete the coeffs array
    for (set=0; set<nSets; set++) {
        delete[] coeffs[set];
    }
}

////////////////////////////////////////
// J evaluation
////////////////////////////////////////

// iDebugEval prints out debugging information in the static evaluation routine.
//    0 - none
//    1 - final value
//    2 - board, final value and all components
// only works with OLD_EVAL set to 1 (slower old version)
const int iDebugEval=0;

INLINE_HINT TCoeff ConfigValue(const TCoeff* __restrict__ pcmove, TConfig config, int map, int offset) {
    TCoeff value=pcmove[config+offset];
    if (iDebugEval>1)
        printf("Config: %5lu, Id: %5hu, Value: %4d\n", config, mapsJ[map].ConfigToID(u2(config)), value);
    return value;
}

INLINE_HINT TCoeff PatternValue(TConfig configs[], const TCoeff* __restrict__ pcmove, int pattern, int map, int offset) {
    TConfig config=configs[pattern];
    TCoeff value=pcmove[config+offset];
    if (iDebugEval>1)
        printf("Pattern: %2d - Config: %5lu, Id: %5hu, Value: %4d (pms %2d, %2d)\n",
                pattern, config, mapsJ[map].ConfigToID(u2(config)), value>>16, (value>>8)&0xFF, value&0xFF);
    return value;
}

INLINE_HINT TCoeff ConfigPMValue(const TCoeff* __restrict__ pcmove, TConfig config, int map, int offset) {
    TCoeff value=pcmove[config+offset];
    if (iDebugEval>1)
        printf("Config: %5lu, Id: %5hu, Value: %4d (pms %2d, %2d)\n",
                config, mapsJ[map].ConfigToID(u2(config)), value>>16, (value>>8)&0xFF, value&0xFF);
    return value;
}


// offsetJs for coefficients
static constexpr int offsetJR1 = 0, sizeJR1 = 6561,
offsetJR2 = offsetJR1 + sizeJR1, sizeJR2 = 6561,
offsetJR3 = offsetJR2 + sizeJR2, sizeJR3 = 6561,
offsetJR4 = offsetJR3 + sizeJR3, sizeJR4 = 6561,
offsetJD8 = offsetJR4 + sizeJR4, sizeJD8 = 6561,
offsetJD7 = offsetJD8 + sizeJD8, sizeJD7 = 2187,
offsetJD6 = offsetJD7 + sizeJD7, sizeJD6 = 729,
offsetJD5 = offsetJD6 + sizeJD6, sizeJD5 = 243,
offsetJTriangle = offsetJD5 + sizeJD5, sizeJTriangle = 9 * 6561,
offsetJC4 = offsetJTriangle + sizeJTriangle, sizeJC4 = 6561,
offsetJC5 = offsetJC4 + sizeJC4, sizeJC5 = 6561 * 9,
offsetJEX = offsetJC5 + sizeJC5, sizeJEX = 6561 * 9,
offsetJMP = offsetJEX + sizeJEX, sizeJMP = 64,
offsetJMO = offsetJMP + sizeJMP, sizeJMO = 64,
offsetJPMP = offsetJMO + sizeJMO, sizeJPMP = 64,
offsetJPMO = offsetJPMP + sizeJPMP, sizeJPMO = 64,
offsetJPAR = offsetJPMO + sizeJPMO; // sizeJPAR = 2;

// value all the edge patterns. return the sum of the values.
static INLINE_HINT TCoeff ValueEdgePatternsJ(const TCoeff* __restrict__ pcmove, TConfig config1, TConfig config2) {
    u4 configs2x5;
    TCoeff value;

    value=0;
    configs2x5=row1To2x5[config1]+row2To2x5[config2];
    value+=ConfigValue(pcmove, configs2x5&0xFFFF, C2x5J, offsetJC5);
    value+=ConfigValue(pcmove, configs2x5>>16,    C2x5J, offsetJC5);
    value+=ConfigValue(pcmove, config1 * 3 +row2ToXX[config2],CR1XXJ, offsetJEX);

    // in J-configs, the values are multiplied by 65536
    //assert((value&0xFFFF)==0);
    return value;
}

// value all the triangle patterns. return the sum of the values.
static INLINE_HINT TCoeff ValueTrianglePatternsJ(const TCoeff* __restrict__ pcmove, TConfig config1, TConfig config2, TConfig config3, TConfig config4) {
    u4 configsTriangle;
    TCoeff value;

    value=0;

    configsTriangle=row1ToTriangle[config1]+row2ToTriangle[config2]+row3ToTriangle[config3]+row4ToTriangle[config4];
    value+=ConfigPMValue(pcmove, configsTriangle&0xFFFF, C4J, offsetJTriangle);
    value+=ConfigPMValue(pcmove, configsTriangle>>16,    C4J, offsetJTriangle);

    return value;
}

static INLINE_HINT CValue ValueJMobs(const CBitBoard &bb, int nEmpty, bool fBlackMove, TCoeff *const pcoeffs, u4 nMovesPlayer, u4 nMovesOpponent) {
// This function implements a linear pattern evaluator. 
//
// Most of the work is in extracting base-3 patterns such as rows, columns,
// diagonals, and certain corner regions. These base-3 values index into the
// pcoeffs array. The sum for the resulting pcoeffs values is the main
// component of the static evaluation.
//
// The other three components of the static evaluation are:
// * mobility (nMovesPlayer and nMovesOpponent are passed to this function)
// * potential mobility (the potential mobility values are baked into the
//   base-3 patterns, as the lower-order bits of the resulting value)
// * parity
// 
// There are several strategies for extracting the patterns:
// * For rows: shift, mask and conversion to base 3 via base2ToBase3Table
//   table lookup - see  BB_EXTRACT_ROW_PATTERN macro
// * For columns, diagonally flip the position and then use a strategy
//   similar to the row one - see BB_EXTRACT_FLIPPED_ROW_PATTERN
// * For most of the diagonals, the magic multiply trick (a.k.a. kindergarten
//   bitboards or bit gather via multiplication) is used to gather the bits,
//   then base2ToBase3Table converts to base 3.
// * For some of the smaller diagonals, the bit gather operation can be
//   combined directly with base 2-to-base 3 conversion, eliminating the need
//   for a table lookup. The mechanism is described here:
//   http://drpetric.blogspot.com/2014/03/bit-gathering-and-base-2-to-base-3.htm
// * For corner regions, table lookups convert the row (or column) base 3
//   patterns to their corner region formats.
//
// The layout of this function may seem "funky". The intent is to minimize the
// number of overlapping live values, in order to reduce the number of
// potential register spills onto the stack (amd64 a.k.a. x86_64 has only 15
// GPRs). That is why we start with diagonals - we only need the diagonals for
// their corresponding pattern values. The rows, on the other hand, are needed
// for corner areas.
// 
    TCoeff value = 0;

    if (iDebugEval>1) {
        cout << "----------------------------\n";
        //bb.Print(fBlackMove);
        cout << (fBlackMove?"Black":"White") << " to move\n\n";
    }

    uint64_t empty = bb.empty;
    uint64_t mover = bb.mover;

    

#define BB_EXTRACT_STEP_PATTERN(START, COUNT, STEP) \
    (base2ToBase3Table[_pext_u64(empty, meta_repeated_bit<uint64_t, (START), (COUNT), (STEP)>::value)] +  \
     2 * base2ToBase3Table[_pext_u64(mover, meta_repeated_bit<uint64_t, (START), (COUNT), (STEP)>::value)])

    const TCoeff* __restrict__ const pD5 = pcoeffs+offsetJD5;
    const TCoeff* __restrict__ const pD6 = pcoeffs+offsetJD6;
    const TCoeff* __restrict__ const pD7 = pcoeffs+offsetJD7;
    const TCoeff* __restrict__ const pD8 = pcoeffs+offsetJD8;

    // Diagonals of type A run NWSE, with a bit step of 9.
    // Type B diagonals run NESW, with a bit step of 7.
    // Diag 8A and 8B
    value += pD8[BB_EXTRACT_STEP_PATTERN(0, 8, 9)];
    uint32_t Diag8B =
        base2ToBase3Table[extract_second_diagonal(empty)] +
        base2ToBase3Table[extract_second_diagonal(mover)] * 2;
    value += pD8[Diag8B]; 
    static constexpr __m256i const_pow2_to_pow3 = (__m256i)(__v32qu) {
      0, 1, 3, 4, 9, 10, 12, 13, 27, 28, 30, 31, 36, 37, 39, 40,
      0, 1, 3, 4, 9, 10, 12, 13, 27, 28, 30, 31, 36, 37, 39, 40
    };
    static constexpr __m256i const_u32_lsbyte_mask = (__m256i)(__v8su) {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    static constexpr __m256i const_diag76_masks_lo = (__m256i)(__v8su) {
      // D7
      meta_u32_extractor<1, 4, 9>::and_mask, meta_u32_extractor<6, 4, 7>::and_mask,
      meta_u32_extractor<8, 3, 9>::and_mask, meta_u32_extractor<15, 3, 7>::and_mask,
      // D6
      meta_u32_extractor<2, 4, 9>::and_mask, meta_u32_extractor<5, 4, 7>::and_mask,
      meta_u32_extractor<16, 2, 9>::and_mask, meta_u32_extractor<23, 2, 7>::and_mask};
    static constexpr __m256i const_diag76_multiplier_lo = (__m256i)(__v8su) {
      // D7
      meta_u32_extractor<1, 4, 9>::multiplier, meta_u32_extractor<6, 4, 7>::multiplier,
      meta_u32_extractor<8, 3, 9>::multiplier, meta_u32_extractor<15, 3, 7>::multiplier,
      // D6
      meta_u32_extractor<2, 4, 9>::multiplier, meta_u32_extractor<5, 4, 7>::multiplier,
      meta_u32_extractor<16, 2, 9>::multiplier, meta_u32_extractor<23, 2, 7>::multiplier};
    static constexpr __m256i const_diag76_shift_lo = (__m256i)(__v8su) {
      // D7
      meta_u32_extractor<1, 4, 9>::shift, meta_u32_extractor<6, 4, 7>::shift,
      meta_u32_extractor<8, 3, 9>::shift, meta_u32_extractor<15, 3, 7>::shift,
      // D6
      meta_u32_extractor<2, 4, 9>::shift, meta_u32_extractor<5, 4, 7>::shift,
      meta_u32_extractor<16, 2, 9>::shift, meta_u32_extractor<23, 2, 7>::shift};
  

    static constexpr __m256i const_diag76_masks_hi = (__m256i)(__v8su) {
      // D7
      meta_u32_extractor<5, 3, 9>::and_mask, meta_u32_extractor<2, 3, 7>::and_mask,
      meta_u32_extractor<3, 4, 9>::and_mask, meta_u32_extractor<4, 4, 7>::and_mask,
      // D6
      meta_u32_extractor<6, 2, 9>::and_mask, meta_u32_extractor<1, 2, 7>::and_mask,
      meta_u32_extractor<2, 4, 9>::and_mask, meta_u32_extractor<5, 4, 7>::and_mask
    };
    static constexpr __m256i const_diag76_multiplier_hi = (__m256i)(__v8su) {
      // D7
      meta_u32_extractor<5, 3, 9>::multiplier, meta_u32_extractor<2, 3, 7>::multiplier,
      meta_u32_extractor<3, 4, 9>::multiplier, meta_u32_extractor<4, 4, 7>::multiplier,
      // D6
      meta_u32_extractor<6, 2, 9>::multiplier, meta_u32_extractor<1, 2, 7>::multiplier,
      meta_u32_extractor<2, 4, 9>::multiplier, meta_u32_extractor<5, 4, 7>::multiplier
    };
    static constexpr __m256i const_diag76_shift_hi = (__m256i)(__v8su) {
      // D7
      meta_u32_extractor<5, 3, 9>::shift, meta_u32_extractor<2, 3, 7>::shift,
      meta_u32_extractor<3, 4, 9>::shift, meta_u32_extractor<4, 4, 7>::shift,
      // D6
      meta_u32_extractor<6, 2, 9>::shift, meta_u32_extractor<1, 2, 7>::shift,
      meta_u32_extractor<2, 4, 9>::shift, meta_u32_extractor<5, 4, 7>::shift
    };

    int mover_lo = static_cast<int>(static_cast<uint32_t>(mover));
    __m256i vect_mover_lo = _mm256_set_epi32(
      mover_lo, mover_lo, mover_lo, mover_lo, mover_lo, mover_lo, mover_lo, mover_lo);

    __m256i d76_mover_diag_lo = 
      _mm256_srlv_epi32(
        _mm256_mullo_epi32(
          const_diag76_multiplier_lo,
          _mm256_and_si256(vect_mover_lo, const_diag76_masks_lo)),
        const_diag76_shift_lo);

    int mover_hi = static_cast<int>(static_cast<uint32_t>(mover >> 32));
    __m256i vect_mover_hi = _mm256_set_epi32(
      mover_hi, mover_hi, mover_hi, mover_hi, mover_hi, mover_hi, mover_hi, mover_hi);
    static constexpr __m256i factor_p3_hi = (__m256i)(__v8su) {
      81, 81, 27, 27, 81, 81, 9, 9};
    __m256i d76_mover_diag_hi = 
      _mm256_srlv_epi32(
        _mm256_mullo_epi32(
          const_diag76_multiplier_hi,
          _mm256_and_si256(vect_mover_hi, const_diag76_masks_hi)),
        const_diag76_shift_hi);

    auto p3_mover_diag_lo = _mm256_shuffle_epi8(const_pow2_to_pow3, d76_mover_diag_lo);
    auto p3_mover_diag_hi = _mm256_shuffle_epi8(const_pow2_to_pow3, d76_mover_diag_hi);
    auto p3_mover = _mm256_add_epi32(p3_mover_diag_lo,  _mm256_mullo_epi32(p3_mover_diag_hi, factor_p3_hi));

    int empty_lo = static_cast<int>(static_cast<uint32_t>(empty));
    __m256i vect_empty_lo = _mm256_set_epi32(
      empty_lo, empty_lo, empty_lo, empty_lo, empty_lo, empty_lo, empty_lo, empty_lo);

    __m256i d76_empty_diag_lo = 
      _mm256_srlv_epi32(
        _mm256_mullo_epi32(
          const_diag76_multiplier_lo,
          _mm256_and_si256(vect_empty_lo, const_diag76_masks_lo)),
        const_diag76_shift_lo);

    int empty_hi = static_cast<int>(static_cast<uint32_t>(empty >> 32));
    __m256i vect_empty_hi = _mm256_set_epi32(
      empty_hi, empty_hi, empty_hi, empty_hi, empty_hi, empty_hi, empty_hi, empty_hi);
    __m256i d76_empty_diag_hi = 
      _mm256_srlv_epi32(
        _mm256_mullo_epi32(
          const_diag76_multiplier_hi,
          _mm256_and_si256(vect_empty_hi, const_diag76_masks_hi)),
        const_diag76_shift_hi);

    __m256i p3_empty_diag_lo = _mm256_shuffle_epi8(const_pow2_to_pow3, d76_empty_diag_lo);
    __m256i p3_empty_diag_hi = _mm256_shuffle_epi8(const_pow2_to_pow3, d76_empty_diag_hi);
    __m256i p3_empty = _mm256_add_epi32(p3_empty_diag_lo,  _mm256_mullo_epi32(p3_empty_diag_hi, factor_p3_hi));
    __m256i d67_pattern = _mm256_add_epi32(p3_empty, _mm256_slli_epi32(p3_mover, 1));

    static constexpr __m256i const_d67_offsets_256 = (__m256i)(__v8su){offsetJD7, offsetJD7, offsetJD7, offsetJD7, offsetJD6, offsetJD6, offsetJD6, offsetJD6};
    __m256i diag_results = _mm256_i32gather_epi32(pcoeffs, _mm256_add_epi32(d67_pattern, const_d67_offsets_256), 4);
    static constexpr __m256i const_diag7_masks_4x_lo = (__m256i)(__v8su) {
      meta_repeated_bit<uint32_t, 1, 4, 9>::value, 0,
      meta_repeated_bit<uint32_t, 6, 4, 7>::value, 0,
      meta_repeated_bit<uint32_t, 8, 3, 9>::value, 0,
      meta_repeated_bit<uint32_t, 15, 3, 7>::value, 0};
    static constexpr __m256i const_diag7_multipliers_4x_lo = (__m256i)(__v8su) {
      meta_base3_u32_hilo<1, 4, 9>::multiplier, 0,
      meta_base3_u32_hilo<6, 4, 7>::multiplier, 0,
      meta_base3_u32_hilo<8, 3, 9>::multiplier, 0,
      meta_base3_u32_hilo<15, 3, 7>::multiplier, 0};
    static constexpr __m256i const_diag7_masks_4x_hi = (__m256i)(__v8su) {
      meta_repeated_bit<uint32_t, 5, 3, 9>::value, 0,
      meta_repeated_bit<uint32_t, 2, 3, 7>::value, 0,
      meta_repeated_bit<uint32_t, 3, 4, 9>::value, 0,
      meta_repeated_bit<uint32_t, 4, 4, 7>::value, 0};
    static constexpr __m256i const_diag7_multipliers_4x_hi = (__m256i)(__v8su) {
      meta_base3_u32_hilo<5, 3, 9>::multiplier, 0,
      meta_base3_u32_hilo<2, 3, 7>::multiplier, 0,
      meta_base3_u32_hilo<3, 4, 9>::multiplier, 0,
      meta_base3_u32_hilo<4, 4, 7>::multiplier, 0};
    static constexpr __m256i mask = (__m256i)(__v8su) {
      0, (1 << 9) - 1, 0, (1 << 7) - 1, 0, (1 << 9) - 1, 0, (1 << 7) - 1};
    static constexpr __m256i factor_p3 = (__m256i)(__v8su) {
      0, 81, 0, 81, 0, 27, 0, 27};

    __m256i mover_lo_4x = _mm256_set_epi64x(static_cast<long long int>(mover), static_cast<long long int>(mover), static_cast<long long int>(mover), static_cast<long long int>(mover));
    __m256i mover_hi_4x = _mm256_set_epi64x(static_cast<long long int>(mover >> 32), static_cast<long long int>(mover >> 32), static_cast<long long int>(mover >> 32), static_cast<long long int>(mover >> 32));
    __m256i empty_lo_4x = _mm256_set_epi64x(static_cast<long long int>(empty), static_cast<long long int>(empty), static_cast<long long int>(empty), static_cast<long long int>(empty));
    __m256i empty_hi_4x = _mm256_set_epi64x(static_cast<long long int>(empty >> 32), static_cast<long long int>(empty >> 32), static_cast<long long int>(empty >> 32), static_cast<long long int>(empty >> 32));

    __m256i mover_lo_base3 = _mm256_and_si256(mask, _mm256_mul_epu32(const_diag7_multipliers_4x_lo, _mm256_and_si256(mover_lo_4x, const_diag7_masks_4x_lo)));
    __m256i mover_hi_base3 = _mm256_mullo_epi32(factor_p3, _mm256_and_si256(mask, _mm256_mul_epu32(const_diag7_multipliers_4x_hi, _mm256_and_si256(mover_hi_4x, const_diag7_masks_4x_hi))));
    __m256i mover_diag_base3 = _mm256_add_epi32(mover_hi_base3, mover_lo_base3);
    __m256i empty_lo_base3 = _mm256_and_si256(mask, _mm256_mul_epu32(const_diag7_multipliers_4x_lo, _mm256_and_si256(empty_lo_4x, const_diag7_masks_4x_lo)));
    __m256i empty_hi_base3 = _mm256_mullo_epi32(factor_p3, _mm256_and_si256(mask, _mm256_mul_epu32(const_diag7_multipliers_4x_hi, _mm256_and_si256(empty_hi_4x, const_diag7_masks_4x_hi))));
    __m256i empty_diag_base3 = _mm256_add_epi32(empty_hi_base3, empty_lo_base3);

    __m256i pattern_7_diag = _mm256_add_epi32(_mm256_srli_epi64(mover_diag_base3, 31), _mm256_srli_epi64(empty_diag_base3, 32));
    __m256i value_7_diag = _mm256_cvtepu32_epi64(_mm256_i64gather_epi32(reinterpret_cast<const int *>(pD7), pattern_7_diag, 4));
    assert(_mm256_extract_epi32(diag_results, 0) == _mm256_extract_epi32(value_7_diag, 0));
    assert(_mm256_extract_epi32(diag_results, 1) == _mm256_extract_epi32(value_7_diag, 2));
    assert(_mm256_extract_epi32(diag_results, 2) == _mm256_extract_epi32(value_7_diag, 4));
    assert(_mm256_extract_epi32(diag_results, 3) == _mm256_extract_epi32(value_7_diag, 6));
    // Diag6 B1 and B2 
    value += pD6[BB_EXTRACT_STEP_PATTERN(5, 6, 7)];
    value += pD6[BB_EXTRACT_STEP_PATTERN(23, 6, 7)];

    // The 0x2030486ca2f300 multiplier will perform the bit gather +
    // base 3 conversion for the 6-long diagonal starting on bit 2, with
    // a step of 9 bits (a.k.a. Diag 6A1).
    // Similar multipliers can be determined for diagonals 6A2, 5A1 and 5A2.
    // However, it is cheaper to reduce 6A2, 5A1 and 5A2 to 6A1 via a
    // shift, rather than load a different constant every single time.
    // The compiler reuses the constants quite effectively.

    uint64_t Diag6A1 =
        (((empty & meta_repeated_bit<uint64_t, 2, 6, 9>::value) * 0x2030486ca2f300) >> 55) +
        2 * (((mover & meta_repeated_bit<uint64_t, 2, 6, 9>::value) * 0x2030486ca2f300) >> 55);
    value += pD6[Diag6A1];
    uint64_t Diag6A2 =
        ((((empty & meta_repeated_bit<uint64_t, 16, 6, 9>::value) >> 14) * 0x2030486ca2f300) >> 55) +
        2 * ((((mover & meta_repeated_bit<uint64_t, 16, 6, 9>::value) >> 14) * 0x2030486ca2f300) >> 55);
    value += pD6[Diag6A2];

    uint64_t Diag5A1 =
         ((((empty & meta_repeated_bit<uint64_t, 3, 5, 9>::value) >> 1) * 0x2030486ca2f300) >> 55) +
         2 * ((((mover & meta_repeated_bit<uint64_t, 3, 5, 9>::value) >> 1) * 0x2030486ca2f300) >> 55);
    value += pD5[Diag5A1];
    uint64_t Diag5A2 =
         ((((empty & meta_repeated_bit<uint64_t, 24, 5, 9>::value) >> 22) * 0x2030486ca2f300) >> 55) +
         2 * ((((mover & meta_repeated_bit<uint64_t, 24, 5, 9>::value) >> 22) * 0x2030486ca2f300) >> 55);
    value += pD5[Diag5A2];

    // The 0x20c49ba2000000 multiplier performs bit gather + base 3 conversion
    // for the 5-long diagonal starting on bit 4, with a step of 7 bits 5(a.k.a.
    // Diag5B1). Similarly, we reduce 5B2 to 5B1 via a shift.
    uint64_t Diag5B1 =  
         ((((empty & meta_repeated_bit<uint64_t, 4, 5, 7>::value)) * 0x20c49ba2000000) >> 57) +
         2 * ((((mover & meta_repeated_bit<uint64_t, 4, 5, 7>::value)) * 0x20c49ba2000000) >> 57);
    value += pD5[Diag5B1];
    uint64_t Diag5B2 = 
         ((((empty & meta_repeated_bit<uint64_t, 31, 5, 7>::value) >> 27) * 0x20c49ba2000000) >> 57) +
         2 * ((((mover & meta_repeated_bit<uint64_t, 31, 5, 7>::value) >> 27) * 0x20c49ba2000000) >> 57);
    value += pD5[Diag5B2];

    assert(pD6[Diag6A1]  == _mm256_extract_epi32(diag_results, 4));
    assert(pD6[BB_EXTRACT_STEP_PATTERN(5, 6, 7)] == _mm256_extract_epi32(diag_results, 5));
    assert(pD6[Diag6A2]  == _mm256_extract_epi32(diag_results, 6));
    assert(pD6[BB_EXTRACT_STEP_PATTERN(23, 6, 7)] == _mm256_extract_epi32(diag_results, 7));
#undef BB_EXTRACT_STEP_PATTERN

    const TCoeff* __restrict__ const pR1 = pcoeffs+offsetJR1;
    const TCoeff* __restrict__ const pR2 = pcoeffs+offsetJR2;
    const TCoeff* __restrict__ const pR3 = pcoeffs+offsetJR3;
    const TCoeff* __restrict__ const pR4 = pcoeffs+offsetJR4;

    __m256i vect_mover_b3 = _mm256_i32gather_epi32(reinterpret_cast<const int *>(base2ToBase3Table), _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(mover)), 4);
    __m256i vect_empty_b3 = _mm256_i32gather_epi32(reinterpret_cast<const int *>(base2ToBase3Table), _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(empty)), 4);
    __m256i vect_pattern = _mm256_add_epi32(vect_empty_b3, _mm256_slli_epi32(vect_mover_b3, 1));
    
    static constexpr __m256i const_row_offsets_256 = (__m256i)(__v8su){offsetJR1, offsetJR2, offsetJR3, offsetJR4, offsetJR4, offsetJR3, offsetJR2, offsetJR1};
    __m256i vect_row_pattern_values_256 = _mm256_add_epi32(value_7_diag, _mm256_i32gather_epi32(pcoeffs, _mm256_add_epi32(vect_pattern, const_row_offsets_256), 4));
    __m256i vect_interm_256 = _mm256_hadd_epi32(vect_row_pattern_values_256, vect_row_pattern_values_256);
    vect_interm_256 = _mm256_hadd_epi32(vect_interm_256, vect_interm_256);
    value += _mm256_extract_epi32(vect_interm_256, 0);
    value += _mm256_extract_epi32(vect_interm_256, 4);
    
    static constexpr __m256i const_triangle_offsets_256 = (__m256i)(__v8su){0, 6561, 2 * 6561, 3 * 6561, 3 * 6561, 2 * 6561, 6561, 0};
    __m256i vect_triangle_rows_256 = _mm256_i32gather_epi32(reinterpret_cast<const int *>(fourRowsToTriangle),
                                                            _mm256_add_epi32(vect_pattern, const_triangle_offsets_256), 4);
    
    vect_interm_256 = _mm256_hadd_epi32(vect_triangle_rows_256, vect_triangle_rows_256);
    vect_interm_256 = _mm256_hadd_epi32(vect_interm_256, vect_interm_256);
    u4 configs_triangle = _mm256_extract_epi32(vect_interm_256, 0);
    value += ConfigPMValue(pcoeffs, configs_triangle&0xFFFF, C4J, offsetJTriangle);
    value += ConfigPMValue(pcoeffs, configs_triangle>>16,    C4J, offsetJTriangle);
    configs_triangle = _mm256_extract_epi32(vect_interm_256, 4);
    value += ConfigPMValue(pcoeffs, configs_triangle&0xFFFF, C4J, offsetJTriangle);
    value += ConfigPMValue(pcoeffs, configs_triangle>>16,    C4J, offsetJTriangle);
    
    TConfig Row0 = _mm256_extract_epi32(vect_pattern, 0);
    TConfig Row1 = _mm256_extract_epi32(vect_pattern, 1);
    TConfig Row6 = _mm256_extract_epi32(vect_pattern, 6);
    TConfig Row7 = _mm256_extract_epi32(vect_pattern, 7);
    
    TConfig valueEdge = ValueEdgePatternsJ(pcoeffs, Row0, Row1) + ValueEdgePatternsJ(pcoeffs, Row7, Row6);
   
#define BB_EXTRACT_FLIPPED_ROW_PATTERN(ROW) \
    (base2ToBase3Table[_pext_u64(empty, meta_repeated_bit<uint64_t, (ROW), 8, 8>::value)] +  \
    2 * base2ToBase3Table[_pext_u64(mover, meta_repeated_bit<uint64_t, (ROW), 8, 8>::value)])

    TConfig Column0 = BB_EXTRACT_FLIPPED_ROW_PATTERN(0);
    value += pR1[Column0];
    TConfig Column1 = BB_EXTRACT_FLIPPED_ROW_PATTERN(1);
    value += pR2[Column1];
    valueEdge += ValueEdgePatternsJ(pcoeffs, Column0, Column1);
    TConfig Column6 = BB_EXTRACT_FLIPPED_ROW_PATTERN(6);
    value += pR2[Column6];
    TConfig Column7 = BB_EXTRACT_FLIPPED_ROW_PATTERN(7);
    value += pR1[Column7];
    valueEdge += ValueEdgePatternsJ(pcoeffs, Column7, Column6);
    value += pR3[BB_EXTRACT_FLIPPED_ROW_PATTERN(2)];
    value += pR3[BB_EXTRACT_FLIPPED_ROW_PATTERN(5)];
    value += pR4[BB_EXTRACT_FLIPPED_ROW_PATTERN(3)];
    value += pR4[BB_EXTRACT_FLIPPED_ROW_PATTERN(4)];
#undef BB_EXTRACT_FLIPPED_ROW_PATTERN


    if (iDebugEval>1)
        printf("Straight lines and corners done. Value so far: %d.\n",value>>16);


    // Take apart packed information about pot mobilities
    unsigned nPMO=(value>>8) & 0xFF;
    unsigned nPMP=value&0xFF;
    if (iDebugEval>1)
        printf("Raw pot mobs: %d, %d\n", nPMO,nPMP);
    nPMO=(nPMO+potMobAdd)>>potMobShift;
    nPMP=(nPMP+potMobAdd)>>potMobShift;
    value>>=16;

    // pot mobility
    value += ConfigValue(pcoeffs, nPMP, PM1J, offsetJPMP);
    value += ConfigValue(pcoeffs, nPMO, PM2J, offsetJPMO);

    if (iDebugEval>1)
        printf("Potential mobility done. Value so far: %d.\n",value);

    value += valueEdge;

    if (iDebugEval>1)
        printf("Edge patterns done. Value so far: %d.\n",value);


    // mobility
    value+=ConfigValue(pcoeffs, nMovesPlayer, M1J, offsetJMP);
    value+=ConfigValue(pcoeffs, nMovesOpponent, M2J, offsetJMO);

    if (iDebugEval>1)
        printf("Mobility done. Value so far: %d.\n",value);

    // parity
    value+=ConfigValue(pcoeffs, nEmpty&1, PARJ, offsetJPAR);

    if (iDebugEval>0)
        printf("Total Value= %d\n", value);

    return CValue(value);
}

// pos2 evaluators
CValue CEvaluator::EvalMobs(const Pos2& pos2, u4 nMovesPlayer, u4 nMovesOpponent) const {
    int nEmpty = pos2.NEmpty();
    return ValueJMobs(pos2.GetBB(), nEmpty, pos2.BlackMove(), pcoeffs[nEmpty], nMovesPlayer, nMovesOpponent);
}
