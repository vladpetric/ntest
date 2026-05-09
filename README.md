# NTest
Ntest is a strong othello program. It can be used in command-line mode or using the graphical viewer NBoard.

Requirements:

* Windows, Mac, or Linux.
* On x86, popcnt instruction. This is available on most Intel CPUs since 2009 and AMD systems since 2008. You will get an error message if trying to run on a computer without popcnt.
* 64 bit builds are highly recommended, though 32 bit builds should work as well.

# Building and quickly running a benchmark

The preferred way of building ntest is through akro build, a C++ build system that I (Vlad Petric) developed.

You should be able to easily install it if you already have a recent Ruby (>= 2.2)  as follows:

``gem install akro``

Then:

``cd src/``

``akro release/ntest.exe``

To quickly run a benchmark:

``./release/ntest.exe t``

The benchmark tests both the solver (from 26 empties), and the midgame searcher (36 empties, depth of 26).

## Running a GGF benchmark with OpenMP

The `t` benchmark mode can also run a GGF game file directly:

```
./release/ntest.exe t /path/to/games.ggf <end-depth> <mid-depth>
```

For example, from `src/`:

```
./release/ntest.exe t /home/vlad/oth/ntest-specv8/data/refspeed/input/Othello.01e4.256.ggf 20 21
```

This runs two benchmark passes:

* the endgame/solve pass at `<end-depth>` empties
* the midgame pass from 36 empties at `<mid-depth>`

When built on Linux, the akro release build enables OpenMP with generic `-fopenmp` flags. Use `OMP_NUM_THREADS` to control parallelism:

```
OMP_NUM_THREADS=4 ./release/ntest.exe t /home/vlad/oth/ntest-specv8/data/refspeed/input/Othello.01e4.256.ggf 20 21
OMP_NUM_THREADS=6 ./release/ntest.exe t /home/vlad/oth/ntest-specv8/data/refspeed/input/Othello.01e4.256.ggf 20 21
OMP_NUM_THREADS=8 ./release/ntest.exe t /home/vlad/oth/ntest-specv8/data/refspeed/input/Othello.01e4.256.ggf 20 21
```

Run the command from `src/` so ntest finds its local `coefficients/` directory.

For x86 builds, cmake should work as well. See BuildREADME.txt

## Other Resources

From the Wayback Machine, [NTest's old homepage](https://web.archive.org/web/20131011003457/http://othellogateway.com/ntest/Ntest/)
