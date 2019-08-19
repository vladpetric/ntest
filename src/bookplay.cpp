// Copyright Chris Welty, Vlad Petric
//  All Rights Reserved
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

#include <sys/mman.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "core/CalcParams.h"
#include "core/Book.h"
#include "core/Cache.h"
#include "core/MPCStats.h"
#include "game/Player.h"
#include "game/Game.h"
#include "n64/flips.h"
#include "n64/utils.h"
#include "options.h"
#include "pattern/FastFlip.h"
#include "pattern/Patterns.h"
#include "Evaluator.h"
#include "PlayerComputer.h"
#include "SmartBook.h"

using namespace std;

static CCache big_cache(16777216); // 512 MiB

class CPlayerWithCache:  public CPlayerComputer {
public:
  CPlayerWithCache(const CComputerDefaults& acd) {
    cd=acd;
    pcp=CCalcParams::NewFromString(cd.sCalcParams);
    madvise(big_cache.buckets, sizeof(CCacheData) * big_cache.nBuckets,
        MADV_HUGEPAGE);

    caches[0]=caches[1]=NULL;
    fHasCachedPos[0]=fHasCachedPos[1]=false;
    std::cout << "status Loading book" << std::endl;
    book=(cd.booklevel!=CComputerDefaults::kNoBook) ? CSmartBook::FindBook(cd.cEval, cd.cCoeffSet, pcp) : NULL;
    eval=CEvaluator::FindEvaluator(cd.cEval, cd.cCoeffSet);
    mpcs=CMPCStats::GetMPCStats(cd.cEval, cd.cCoeffSet, std::max(cd.iPruneMidgame, cd.iPruneEndgame));
    fAnalyzingDeferred=false;

    // get saved-game file name
    std::ostringstream os;
    os << fnBaseDir << "results/" << cd.cEval << cd.cCoeffSet << '_' << *pcp << ".ggf";
    m_fnSaveGame=os.str();
    
    toot=false;
    
    // calculate computer's name
    std::ostringstream osName;
    pcp->Name(osName);
    m_sName=osName.str();

    if (!mpcs) cd.iPruneMidgame=cd.iPruneEndgame=0;


    std::cout << "status Negamaxing book" << std::endl;
    SetupBook(cd.booklevel==CComputerDefaults::kNegamaxBook);
    std::cout << "status" << std::endl;
  }
protected:
  CCache* GetCache(int iCache) override {
    return &big_cache;
  }
};

void Init() {
  setbuf(stdout, 0);
  srand(static_cast<unsigned int>(time(0)));
  initFlips();
  InitFastFlip();
  InitConfigToPotMob();
  cout << setprecision(3);
  cerr << setprecision(3);

  void InitFFBonus();
  InitFFBonus();
  void InitForcedOpenings();
  InitForcedOpenings();
}

// To satisfy linker in debug mode
bool HasInput() {
    return false;
}

int main(int argc, char **argv) {
    cout << "Ntest version as of " << __DATE__ << "\n";
    cout << "Copyright 1999-2014 Chris Welty and Vlad Petric\nAll Rights Reserved\n\n";
    if (argc != 2) {
      cerr << "Usage: " << argv[0] << " <initial file>"  << std::endl;
    }
    try {
      mlockall(MCL_CURRENT | MCL_FUTURE);
      cout << "Done locking pages" << std::endl;
      for (unsigned depth: {18}) {
        dGHz = 2.8;
        maxCacheMem = 2ULL << 30; //2GiB 
        Init();
        CComputerDefaults cd1;
        char buff[5];
        snprintf(buff, 5, "s%d", depth);
        cd1.sCalcParams = buff;
        cd1.cEval = 'J';
        cd1.cCoeffSet = 'A';
        cd1.vContempts[0] = 0;
        cd1.vContempts[1] = 0;
        cd1.iPruneEndgame = 5;
        cd1.iPruneMidgame = 4;
        cd1.nRandShifts[0] = 2;
        cd1.nRandShifts[1] = 2;
        cd1.iEdmund = 1;
        cd1.fsPrint = 0;
        cd1.fsPrintOpponent = 0;
        cd1.booklevel = CComputerDefaults::kBook;

        fPrintExtensionInfo = false;
        fPrintBoard = false;
        fPrintAbort = false;
        extern bool fPrintCorrections;
        fPrintCorrections = false;
        
        CPlayer* p0 = new CPlayerWithCache(cd1);
        for (unsigned i = 0; i < 100000; ++i) {
            CGame(p0, p0, 15 * 60, argv[1]).Play();
            cout << "\n\n\n Done with depth " << depth << " Game " << i << "\n\n" << endl;
        }
        delete p0;
    }
  } catch(std::string exception) {
    cout << exception << "\n";
    return -1;
  }
}
