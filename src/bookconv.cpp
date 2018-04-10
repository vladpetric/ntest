// Copyright Chris Welty, Vlad Petric
//  All Rights Reserved
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "core/CalcParams.h"
#include "core/Book.h"
#include "game/Player.h"
#include "game/Game.h"
#include "n64/flips.h"
#include "n64/utils.h"
#include "options.h"
#include "pattern/FastFlip.h"
#include "pattern/Patterns.h"
#include "PlayerComputer.h"

using namespace std;

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
void ReadMachineParameters(istream& isBig) {
    string sLine;
    getline(isBig, sLine);
    std::istringstream is(sLine.c_str());

    dGHz=2.5;

    is >> maxCacheMem >> dGHz;

    // convert MBytes to bytes in maxCacheMem
    maxCacheMem<<=20;

}
void ReadParameters(CComputerDefaults& cd1, CComputerDefaults& cd2) {
    string fn("./");
    fn+="parameters.txt";
    std::ifstream isParams(fn);
    if (!isParams.good()) {
    	throw std::string("Can't read in parameters file ")+fn;
    }
    
    ReadMachineParameters(isParams);
    isParams >> cd1 >> cd2;
    while (1) {
    	std::string sLine;
    	if (!getline(isParams, sLine))
    		break;
    	std::istringstream is(sLine);

    	std::string sParamName;
    	is >> sParamName;
    	if (sParamName=="BookWriteFormat") {
    		is >> CBook::s_iBookWriteFormat;
    	}
    }
}

// To satisfy linker in debug mode
bool HasInput() {
    return false;
}

int main(int argc, char **argv) {
    cout << "Ntest version as of " << __DATE__ << "\n";
    cout << "Copyright 1999-2014 Chris Welty and Vlad Petric\nAll Rights Reserved\n\n";
    try {
        maxCacheMem = 4ULL << 30; //4GB 
        Init();
    	CComputerDefaults cd1, cd2;
        ReadParameters(cd1, cd2);

        CBook book;
        book.ReadStructFile("coefficients/inline.JA_s26.book");
    } catch(std::string exception) {
    	cout << exception << "\n";
    	return -1;
    }
}
