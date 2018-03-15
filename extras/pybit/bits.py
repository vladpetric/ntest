from __future__ import print_function

import sys
def display(number, out=sys.stdout):
  for i in range(8):
    for j in range(8):
      if (number & 1) == 1:
        print(1, end="", file=out)
      else:
        print(0, end="", file=out)
      number = number >> 1
      print(' ', end="", file=out)
    print(file=out)
