<pre style="text-align:center;">
<p align="center">
    █     ▀██▀▀█▄       █     ▀██    ██▀ 
   ███     ██   ██     ███     ███  ███  
  █  ██    ██    ██   █  ██    █▀█▄▄▀██  
 ▄▀▀▀▀█▄   ██    ██  ▄▀▀▀▀█▄   █ ▀█▀ ██  
▄█▄  ▄██▄ ▄██▄▄▄█▀  ▄█▄  ▄██▄ ▄█▄ █ ▄██▄ 

v0.10.0

<b>Chaos based random number generation</b>
Use at your own risk. Criticism and suggestions are welcome
</pre>         

ADAM is a cryptographically secure pseudorandom number generator (CSPRNG) originally inspired by ISAAC64. The heart of the generator is an optimized adaptation of the algorithm described in the paper [François M et al. Pseudo-random number generator based on mixing of three chaotic maps. Commun Nonlinear Sci Numer Simulat (2013)](https://doi.org/10.1016/j.cnsns.2013.08.032), which uses a chaotic function over multiple iterations to produce random bits with strong cryptographic properties. ADAM couples parts of ISAAC’s logic with this algorithm where it is applicable to form a compact number generation scheme that’s easy to use, tune, and test.

Also, just like ISAAC, ADAM is a backronym that describes its operations:

**A** ccumulate a set of chaotic seeds<br>
**D** iffuse the work buffer with logic from ISAAC<br>
**A** pply the chaotic function<br>
**M** ix the work and state buffers to produce the output vector

The chaotic algorithm behind the number generation process is detailed in the paper above, and you can learn more about ISAAC and its brilliant author Bob Jenkins [here](http://burtleburtle.net/bob/rand/isaacafa.html). 

## TABLE OF CONTENTS
1. [Features](#features)
2. [Installation](#install)
3. [Command Line](#cli)
4. [Library](#lib)
5. [Security](#secure)
6. [Testing](#test)
7. [Contributing](#contrib)

## <a id="features"></a> FEATURES

- Statespace: 2⁵¹⁵²⁰
- Multi-cyclic: minimum cycle length (period) is 2²⁵⁶
- 2¹⁴⁰⁶ possible pseudorandom sequences per cycle
- Free choice of input vector + seeding routine which really facilitates experimentation         
- Avoids brute force and differential attacks
- Two (optional) input parameters: one 256-bit seed and one 64-bit nonce
- Space Complexity: O(N)
- Output sequence is irreversible
- Natively examine statistical properties of a generated sequence with the in-house test suite
- Easy interface for bit generation in both ASCII and binary form. Output up to 100GB at a time.
- Alternatively, stream bits directly to the `stdin` of your own programs, RNG test suites, etc.
- Extract different widths of integers from the buffer in decimal, hexadecimal, or octal format
- Generate RFC 4122 compliant UUIDs
- View all generated integers at once with your width of choice
- Uses SIMD acceleration where applicable (ARM NEON, AVX/AVX2, or AVX-512F)
- User-friendly API definitions for usage as a library
- Floating point number generation, with configurable precision for output and scaling factor support
- Generate up to 1 billion integers/doubles per API call
- Passes all empirical test suites that I am aware of (see table below)

### Coming Soon!
- FFI bindings!
- Benchmarks!
- Pre-compiled binaries?
- Multi-threading the examination test suite?

## <a id="install"></a> INSTALLATION

ADAM was developed for macOS and Linux systems and requires clang or gcc. It may be possible to compile on other operating systems but I haven't checked or configured the build for other systems/distros. The macOS version makes use of ARM NEON intrinsics, but the equivalent logic is implemented using AVX/AVX2 or AVX-512F. Either way, support for at least one of these sets of intrinsics is required. 

By default the executable, library, and header will be available in `./build` but you can change this by tweaking the `BUILD_DIR` variable in the Makefile.

```
git clone https://github.com/pre-eth/adam.git
cd adam
make
cd build
./adam -h
```

And you should be good to go! 
 
## <a id="cli"></a> COMMAND LINE

If you run `adam` with no arguments, you will get one randomly generated unsigned 64-bit number.

Here's the full command summary:

<pre>
    adam [-h|-v|-b] [-s[seed?]] [-n[nonce?]] [-dxof] [-w width] [-m multiplier]
         [-p precision] [-a multiplier] [-e multiplier] [-r results] [-u[amount?]]

                                           [OPTIONS]

    -h    Get command summary and all available options
    -v    Version of this software (v0.9.0)
    -s    Get the seed for the generated buffer (no parameter), or provide your own. Seeds
          are reusable but should be kept secret
    -n    Get the nonce for the generated buffer (no parameter), or provide your own. Nonces
          should be unique per seed and kept secret
    -u    Generate a universally unique identifier (UUID). Optionally specify a number of
          UUID's to generate (max 1000)
    -r    The amount of numbers to generate and return, written to stdout. Must be within
          max limit for current width (see -d)
    -d    Dump entire buffer using the specified width (up to 256 u64, 512 u32, 1024 u16, or
          2048 u8)
    -w    Desired alternative size (u8, u16, u32) of returned numbers. Default width is u64
    -b    Just bits... literally. Pass the -f flag beforehand to stream random doubles instead
          of integers
    -a    Write an ASCII or binary sample of bits/doubles to file for external assessment.
          You can choose a multiplier to output up to 100GB of bits, or 1 billion doubles
          (with optional scaling factor), at a time
    -e    Examine a sample of 1MB with the ENT framework and some other statistical tests to
          reveal properties of the output sequence. You can choose a multiplier within [1,
          1000000] to examine up to 100GB at a time
    -x    Print numbers in hexadecimal format with leading prefix
    -o    Print numbers in octal format with leading prefix
    -f    Enable floating point mode to generate doubles in (0.0, 1.0) instead of integers
    -p    How many decimal places to print when printing doubles. Must be within [1, 15].
          Default is 15
    -m    Multiplier for randomly generated doubles, such that they fall in the range (0,
          MULTIPLIER)
</pre>

Not too hard to mess around with. Here's some basic examples, for reference:

### Generate 256 unsigned 64-bit integers and dump to stdout

`adam -d`

### Generate and dump whole buffer as unsigned 32-bit integers

`adam -w 32 -d`

### Return 327 unsigned 32-bit integers in hexadecimal form

`adam -w 32 -r 327 -x`

### Generate 12 UUIDs

`adam -u12`

### Enable floating point mode to get a random double

`adam -f`

### Enable floating point mode with 12 digits of precision

`adam -p 12`

**Note:** Setting the precision is enough to invoke floating point mode, so the -f option here would be redundant

### Examine a 1GB generated sequence 

`adam -e 1000`

### Get the seed for the next run

`adam -s`

### Get the seed and nonce for the next run

`adam -s -n`

### Set the seed and nonce 

`adam -smyseed.bin -n<NONCE>`

**Note:** You can set just one of these if you want, no requirement to set both.

### Pipe bits continuously to a program

`adam -b | ./someprog`

### Generate a double within (0.0, 3000000000.0)

`adam -m 3000000000`

**Note:** Setting a scaling factor is enough to invoke floating point mode, so the -f option here would be redundant

### Generate 6 doubles within (0.0, 100.0) with 3 digits of precision

`adam -m 100 -p 3 -r 6`

### Write 500MB of integer data to file in binary mode (interactive)

```
adam -a
File name: testfile
Output type (1 = ASCII, 2 = BINARY): 2
Sequence Size (x 1MB): 500

Generated 4000000000 bits and saved to testfile
```

### Write 1000000 doubles between (0.0, 1.0) to file in ASCII mode (interactive)

```
adam -f -a
File name: testfile_float
Output type (1 = ASCII, 2 = BINARY): 1
Sequence Size (x 1000): 1000

Generated 1000000 doubles and saved to testfile_float
```

**Note:** When outputting doubles in ASCII mode, the current precision value is used when writing the doubles to file.

### Write 1000000 doubles between (0.0, 5000.0) to file in binary mode with specified seed (interactive)

```
adam -smyseed.bin -m 5000 -a
File name: testfile_float5000
Output type (1 = ASCII, 2 = BINARY): 2
Sequence Size (x 1000): 1000

Generated 1000000 doubles and saved to testfile_float5000
```

## <a id="lib"></a> LIBRARY

Aside from the CLI tool, you'll find `build/adam.h` and `build/libadam.a`, allowing you to integrate ADAM into your own code! 

The standard API is designed for those who don't want to write a lot of number handling boilerplate themselves and expect some basic quality-of-life functions. So this API has been designed for ergonomics and versatility.

The standard API has 11 functions:

```c
adam_data                          adam_setup(unsigned long long *seed, unsigned long long *nonce);
unsigned long long *               adam_seed(const adam_data data);
unsigned long long *               adam_nonce(const adam_data data);
const unsigned long long *         adam_buffer(const adam_data data, const bool force_regen);
unsigned long long                 adam_int(adam_data data, unsigned char width, const bool force_regen);
double                             adam_dbl(adam_data data, const unsigned long long scale, const bool force_regen);
int                                adam_fill(adam_data data, void *buf, unsigned char width, const unsigned long long amount);
int                                adam_dfill(adam_data data, double *buf, const unsigned long long multiplier, const unsigned long amount);
void *                             adam_choice(adam_data data, void *arr, const unsigned long long size);
unsigned long long                 adam_stream(adam_data data, const unsigned long long output, const char *file_name);
void                               adam_cleanup(adam_data data);
```

The API is stable - new functions might be introduced, but these ones and their signatures will never change.

Documentation will be available after compilation in `build/adam.h`. 

## <a id="secure"></a> But is it REALLY secure?

“Proving” security is a very difficult thing to do, and a good deal of cryptanalysis is needed before domain wide recognition for the strength of any cryptographic algorithm is gained. That’s why the saying [“Don’t Roll Your Own Crypto”](https://security.stackexchange.com/questions/18197/why-shouldnt-we-roll-our-own) exists.

ADAM has passed the [NIST Test Suite for Random Bit Generation SP 800-22](https://csrc.nist.gov/publications/detail/sp/800-22/rev-1a/final) just like the original algorithm in the paper, along with other more challenging test suites mentioned below. Up to 16TB (2⁴⁴ values) has been tested so far. However, conducting specialized cryptographic testing is required before confident security guarantees can be made. This is just a toy RNG for now, and for production use I strongly recommend using something more thoroughly vetted by the field like [ChaCha20](https://datatracker.ietf.org/doc/html/rfc7539). The next major goal for ADAM after confirming statistical quality will be pursuing crytographic validation and study.

While passing one or even all of these test suites doesn't guarantee that a RNG is cryptographically secure, it follows that a CSPRNG will pass these tests, so they nonetheless provide a measuring stick of sorts to reveal flaws in the design and various characteristics of the randomness of the generated numbers. If you run these tests yourself, you might see false-positives that are retried or some p-values marked as "unusual" - this is to be expected and is a consequence of running many tests over and over, because if the output is really random, then there will inevitably be some weaker outcomes.

The key things to look for are:
- The output is unpredictable unless you know the input parameters, with no apparent bias
- Internal state cannot be reverse engineered from the output
- The statistical quality is high the overwhelming majority of the time (i.e. no consistently suspicious test results and *definitely* no hard failures, ever)
- There is mathematical basis for at least some of the ideas behind the security proposition
- Implementation tradeoffs for speed and performance have negligible, if any, empirical impact on the output's cryptographic strength

The CACert Research Lab provides a [public record](https://www.cacert.at/cgi-bin/rngresults) of the properties of different RNGS available today. You can see how ADAM stacks up against other RNGs from a naive run of ENT and Diehard.

## <a id="test"></a> TESTING

Testing is easy with the `-a` option. The minimum number of bits that can be outputted is 1MB. You can provide a multiplier within [1, 1000] as mentioned above, and then write those bits to a text or binary file which can be supplied to any of the testing frameworks listed below, a different framework, or your own RNG tests!

For piping data directly into a testing program, use the `-b` option.

Click on a test to learn more.

| Status            | Name        | Description | 
| ------------------| ----------- | ----------- | 
| ✅ **PASS** | [ent](https://www.fourmilab.ch/random) | Calculates various values for a supplied pseudo random sequence like entropy, arithmetic mean, Monte Carlo value for pi, and more. Included with the CLI tool
| ✅ **PASS** | [NIST STS](https://csrc.nist.gov/projects/random-bit-generation/documentation-and-software) | Set of statistical tests of randomness for generators intended for cryptographic applications. The general standard.
| ✅ **PASS** | [Dieharder](https://webhome.phy.duke.edu/~rgb/General/dieharder.php) | Cleaned up version of George Marsaglia's DIEHARD suite plus the STS tests from above and RGB tests. Mostly of historical interest.
| ✅ **PASS** | [testunif - gjrand](https://gjrand.sourceforge.net) | Extensive tests not within DIEHARD or NIST that analyze the randomness of bit sequences. Very particular and supposedly pretty tough!
| ✅ **PASS** | [testfunif - gjrand](https://gjrand.sourceforge.net) | Another series of tests specifically for analyzing the uniformity of double precision floating point numbers in [0.0, 1.0).
| ✅ **PASS** | [CACert](https://www.cacert.at/cgi-bin/rngresults) | Cryptographic certificate provider that maintains test results of various RNGs. ADAM is among the top 8 crypto RNG entries which had a flawless test run.
| ✅ **PASS** | [Rabbit - TestU01](http://simul.iro.umontreal.ca/testu01/tu01.html) | For analyzing bitstreams. Treats input as one large binary sequence and applies 40 different statistical tests.
| ✅ **PASS** | [SmallCrush - TestU01](http://simul.iro.umontreal.ca/testu01/tu01.html) | 10 different tests. Small but effective at spotting deficiencies missed by other test suites. Lots of good generators fail this one.
| ✅ **PASS** | [Crush - TestU01](http://simul.iro.umontreal.ca/testu01/tu01.html) | 96 different tests. Very thorough and stringent, requires around 2³⁵ numbers in total. 
| ✅ **PASS** | [BigCrush - TestU01](http://simul.iro.umontreal.ca/testu01/tu01.html) | 106 different tests. Even more stringent, requires "close to" 2³⁸ numbers in total. 
| ✅ **PASS** | [PractRand](https://sourceforge.net/projects/pracrand/) | Final Boss - The most rigorous and effective test suite for PRNGs available. Can test unlimited lengths of sequences - ADAM has been tested up to 16TB!

For details on how to run the tests above, please refer to [TESTING.md](TESTING.md). Some results are available in the [tests](tests) directory, with the exception of ENT because it's built into the `-e` option.

## <a id="contrib"></a> CONTRIBUTING

I started ADAM after looking at the source of ISAAC while I was learning C. I didn't intend to make anything groundbreaking or crazy, but I'm open to all cryptanalysis that can increase the cryptographic strength of this generator. Feel free to reach out to me or open an issue for any ideas/critiques/statistics you may have that can improve the implementation.

As far as code contributions though, I'd like to maintain the project myself for now as a continued learning opportunity. If I open the codebase up in the future I will amend the README, but as of now I'm not looking for other developers.

<img align="right" src="https://github.com/pre-eth/adam/assets/119128171/f1539a5d-985c-47e5-8145-4843764666af" />
