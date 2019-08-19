#!/usr/bin/env python3
# flip_gen.py - mostly based off flips.cpp/h and utils.h
U32 = 0xFFFFFFFF
U64 = 0xFFFFFFFFFFFFFFFF

NDIAGS = 11

MaskA    = 0x0101010101010101
MaskA1H8 = 0x8040201008040201
MaskA8H1 = 0x0102040810204080

def u64(v):
  return v & U64
def u32(v):
  return v & U32

def bit_set(index, bits):
  assert index >= 0
  return (bits >> index) & 1 == 1

def bit_clear(index, bits):
  assert index >= 0
  return (bits >> index) & 1 == 0

def mask(square):
  return 1 << square

def col(square):
  return square & 7

def row(square):
  return square >> 3

def neighbors():
  neighbors = []
  for sq in range(64):
    m = mask(sq) 
    if col(sq) > 0:
      m = m | (m >> 1)
    if col(sq) < 7:
      m = m | (m << 1)

    m = m | (m >> 8) | (m << 8)
    neighbors.append(u64(m & ~mask(sq)))
  return neighbors

def outsides():
  outsides = [[0] * 256 for _ in range(8)]
  for bitPattern in range(256):
    for index in range(8):
      outside = 0
      for i in range(index - 1, -1, -1):
        if bit_clear(i, bitPattern):
          outside = outside | (1 << i)
          break
      for i in range(index + 1, 8):
        if bit_clear(i, bitPattern):
          outside = outside | (1 << i)
          break
      outsides[index][bitPattern] = outside
  return outsides

def counts_and_insides():
  counts   = [[0] * 256 for _ in range(8)]
  insides  = [[0] * 256 for _ in range(8)]
  for bitPattern in range(256):
    for index in range(8):
      count = 0
      inside = 0
      insideLeft = 0
      for i in range(index - 1, -1, -1):
        if bit_set(i, bitPattern):
          count += index - i - 1
          inside = inside | insideLeft
          break
        else:
          insideLeft = insideLeft | (1 << i)

      insideRight = 0
      for i in range(index + 1, 8):
        if bit_set(i, bitPattern):
          inside = inside | insideRight
          count += i - index - 1
          break
        else:
          insideRight = insideRight | (1 << i)
      counts[index][bitPattern] = count
      insides[index][bitPattern] = inside
  return counts, insides

def row_flips():
  rf = [[0] * 256 for _ in range(8)]
  for bitPattern in range(256):
    for index in range(8):
      rf[index][bitPattern] = u64(bitPattern << ((index * 8) % 64))
  return rf

def col_flips():
  cf = [[0] * 256 for _ in range(8)]
  for bitPattern in range(256):
    for index in range(8):
      cf[index][bitPattern] = u64(((bitPattern * 0x02040810204081) & MaskA) << index)
  return cf

def signed_left_shift(v, s):
  if s > 0:
    return v << (s % 64)
  else:
    return v >> ((-s) % 64)

def d9_flips():
  d9 = [[0] * 256 for _ in range(NDIAGS)]
  for bitPattern in range(256):
    for index in range(NDIAGS):
      pattern = (bitPattern * MaskA) & MaskA1H8
      diff = index - 5
      d9[index][bitPattern] = u64(signed_left_shift(pattern, diff * 8))
  return d9

def d7_flips():
  d7 = [[0] * 256 for _ in range(NDIAGS)]
  for bitPattern in range(256):
    for index in range(NDIAGS):
      pattern = (bitPattern * MaskA) & MaskA8H1
      diff = index - 5
      d7[index][bitPattern] = u64(signed_left_shift(pattern, diff * 8))
  return d7

def main():
  for pl in d7_flips():
    for v in pl:
      print("0x%x " % v, end="")
    print()
  print()

if __name__ == "__main__":
  main()
