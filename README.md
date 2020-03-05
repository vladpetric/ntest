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

``
cd src/

akro release/ntest.exe
``

To quickly run a benchmark:

``./release/ntest.exe t``

The benchmark tests both the solver (from 26 empties), and the midgame searcher (36 empties, depth of 26).

For x86 builds, cmake should work as well. See BuildREADME.txt

## Other Resources

From the Wayback Machine, [NTest's old homepage](https://web.archive.org/web/20131011003457/http://othellogateway.com/ntest/Ntest/)
