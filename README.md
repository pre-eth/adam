<pre style="text-align:center;">
<p align="center">
    █     ▀██▀▀█▄       █     ▀██    ██▀ 
   ███     ██   ██     ███     ███  ███  
  █  ██    ██    ██   █  ██    █▀█▄▄▀██  
 ▄▀▀▀▀█▄   ██    ██  ▄▀▀▀▀█▄   █ ▀█▀ ██  
▄█▄  ▄██▄ ▄██▄▄▄█▀  ▄█▄  ▄██▄ ▄█▄ █ ▄██▄ 

v0.1.0

A SIMD accelerated pseudo-random number generator inspired by the code of ISAAC, by Bob
Jenkins. Added some of my own twists and turns to experiment. I did this just for fun and 
learning, not to set any records or make anything groundbreaking. That being said, being able 
to rightfully call this a CSPRNG is my chief goal.

<b>Use at your own risk</b>. Criticism and suggestions are welcome.</p></pre>                                   

You can read more information about ISAAC [here](http://burtleburtle.net/bob/rand/isaacafa.html).

Check out the rest of Bob's site too, it's a treasure trove of interesting information!

Currently, I'm tweaking the algorithm of ADAM to pass the NIST Test Suite for Random Bit Generation [SP 800-22 Rev. 1a](https://csrc.nist.gov/publications/detail/sp/800-22/rev-1a/final). After that, the 
next goal will be to pass Dieharder, and then hopefully other suites of generator tests like PractRand. While passing one or even all of these test suites doesn't guarantee that a RNG is cryptographically secure, it follows that a CSPRNG will pass these tests, so they provide a measuring stick of sorts.


# INSTALLATION

ADAM supports x86-64 based Linux systems. SSE, AVX, AVX2, and optionally AVX-512
intrinsics support is required, along with CMake >= 3.10. Then, inside the repo:

```
cd build
cmake .. && make
./adam -h
```

And you should be good to go! 

ADAM was developed on Fedora, so it should work fine on RHEL flavors,
but as of now I have not been able to test on other distros.

# USAGE
If you run `adam` with no arguments, you will get one randomly generated 64-bit number.

Or you can also provide the following options:

    -h      Get all available options
    -v      Version of this software
    -u      Set the uniform distributor's bitwise right shift depth (default 3, max 8)
    -n      Number of results to return (default 1, max 256)
    -p      Desired size (8, 16, 32) of returned numbers if you need less precision than 64-bit
    -d      Dump all currently generated numbers, separated by spaces
    -b      Just bits. Literally. Optionally, you can provide a certain limit of N bits
    -a      Assess a sample of 100000000 bits (100 MB) written to a filename you provide
    -s      Set the seed (u64). If no argument is provided, returns seed for current buffer
    -l      Live stream of continuously generated numbers

# ALGORITHM

The name comes from the biblical figure Adam (a play on words since I 
came up with this while studying the source of ISAAC64). It is also a 
backronym like ISAAC that explains its process: **A**CCUMULATE, **D**IFFUSE 
**A**SSIMILATE, and **M**ANGLE

## I. ACCUMULATE

A seed can be set programmatically or on the command line. If no seed is 
provided, a true random seed is queried via RDRAND. This seed is used to 
initially fill the buffer, before a set of initialization vectors is permuted
with the same mixing logic from ISAAC64, and then those vectors themselves are
mixed into the buffer. Each vector is inserted twice - once in the top half,
once in the bottom half.

The 8 IV's are derived from the following verse:

> ***"Be fruitful and multiply, and replenish the earth (Genesis 1:28)"***

(Which can also be considered a TL;DR for the whole algorithm in case you
don't want to keep reading.)

A set of 8 initial addends are set to the Golden Ratio (GR), per Bob's original 
implementation. They are also inserted twice like above and distributed across
the top and bottom half.

The initial configuration of the buffer looks like this:

## II. DIFFUSE

First, we permute the IV's and GR values 4x each with the same mixing
logic from ISAAC64. 4 rounds were chosen because that's the same amount of initial
permutation Bob used.

Once they've been permuted themselves, SIMD registers are used to sum different groups
of contiguous bits, with an additional permutation applied to the IV and GR locations
every iteration. These SIMD registers that store the results are written back to the
`frt` buffer in reverse order.

Alternating segments per column are swapped from the top and bottom halves of the
buffer like so:

## III. ASSIMILATE

This phase begins by applying a compression permutation. More rigorously, this means
that a certain subset of bits are selected AND order is changed. So bitwise manipulation
is used for the first requirement and then results are written back in reverse order 
like before for the second requirement:


The right shift parameter can be configured programmatically or on the command line.

Then, the permutation logic of ISAAC64 is extended to 16 integers at a time, and used to
thoroughly blend the top and bottom halves. We now have a rudimentary buffer of 256
64-bit randomly generated integers, but we have some work left before we can use them:

## IV. MANGLE

The last step makes use of the same quarter rounding functions used by the ChaCha
derived BLAKE2b hash function. These are designed to operate on 16 blocks of 
64-bit integers, so they seamlessly integrate with ADAM's operations. 2 rounds are 
applied per 16 integer block on columns and diagonal groups of 4. 

And with that, the user now has a pool of 256 cryptographically generated random 
numbers, which can be retrieved and used upon will! 

Bit shifting is used to satisfy arbitrary precision needs from the user. Each location
is zeroed out after its value is retrieved, so the original isn't retained. 

# TESTS

To run with the NIST Test Suite, use `adam -a > bits` to output a sample of 100 bit
streams to a text file for testing. After that, running the tests is as simple as
loading the file and letting the statistical tests work through the data. This may
take a while. Currently, it passes a good number of the tests but fails other ones
spectacularly, so I'm in the process of figuring out why.   

# LICENSE: MIT

Copyright © 2022 Preeth Vijay

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, 
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to the following 
conditions:

The above copyright notice and this permission notice shall be included in all copies 
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT 
OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
