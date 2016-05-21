#pragma once

#include "Moves.h"
#include "QPositionTest.h"
#include "StoreTest.h"

inline void testCore() {
	void TestBitBoard();
	void TestBook();

	TestBitBoard();
	TestBook();
	TestQPosition();
	CMove::Test();
	CMoves::Test();

	void TestStore();
	TestStore();
}
