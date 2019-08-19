#include "n64/test.h"
#include "n64/flips.h"

#include <cstdio>

int main() {
	initFlips();
//  for (int i = 0; i < 64; ++i) {
//    fprintf(stdout, "0x%lx\n", neighbors[i]);
//  }
  for (int i = 0; i < nDiagonals; ++i) {
    for (int j = 0; j < 256; ++j) {
      fprintf(stdout, "0x%lx ", d7Flips[i][j]);
    }
    fprintf(stdout, "\n");
  }
  fprintf(stdout, "\n");
/*
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 256; ++j) {
      fprintf(stdout, "0x%x ", insides[i][j]);
    }
    fprintf(stdout, "\n");
  } */
}
