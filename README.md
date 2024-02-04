<pre style="text-align:center;">
<p align="center">
    █     ▀██▀▀█▄       █     ▀██    ██▀ 
   ███     ██   ██     ███     ███  ███  
  █  ██    ██    ██   █  ██    █▀█▄▄▀██  
 ▄▀▀▀▀█▄   ██    ██  ▄▀▀▀▀█▄   █ ▀█▀ ██  
▄█▄  ▄██▄ ▄██▄▄▄█▀  ▄█▄  ▄██▄ ▄█▄ █ ▄██▄ 

v1.4.0

<b>Use at your own risk</b>. Criticism and suggestions are welcome.
</pre>         

ADAM is an actively developed cryptographically secure pseudorandom number generator (CSPRNG) originally inspired by ISAAC64. At the heart of the generator is an optimized implementation of the algorithm described in the paper [François M et al. Pseudo-random number generator based on mixing of three chaotic maps. Commun Nonlinear Sci Numer Simulat (2013)](https://doi.org/10.1016/j.cnsns.2013.08.032), which uses a chaotic function over multiple iterations to produce random bits with strong cryptographic properties. ADAM incorporates parts of ISAAC’s logic into this algorithm where it is applicable to form a compact number generation scheme that’s easy to use, tune, and test.

Also, just like ISAAC, ADAM is a backronym that describes its steps:

**A** ccumulate content for the input vector and set of chaotic seeds <br>
**D** iffuse the buffer with logic adapted from ISAAC <br>
**A** pply the necessary iterations of the chaotic function <br>
**M** ix the chaotic maps in the buffer together to produce the output vector

You can find a deeper dive into the algorithm behind the number generation process in the paper above, and you can learn more information about ISAAC and its brilliant author Bob Jenkins [here](http://burtleburtle.net/bob/rand/isaacafa.html).

## FEATURES

- Avoids brute force and differential attacks
- Only two (optional) input parameters: one 256-bit seed and one 64-bit nonce
- Space Complexity: O(N)
- Output sequence is irreversible
- Algorithm requires no heap allocations
- Algorithm requires no external dependencies
- Generator reseeds itself
- Natively examine statistical properties of generated sequences
- Easy interface for bit generation in both ASCII and binary form. Output up to 1GB at a time.
- Alternatively, stream bits directly to the `stdin` of your own programs, RNG test suites, etc.
- Extract different widths of numbers from the buffer in decimal, hexadecimal, or octal format
- Generate RFC 4122 compliant UUID’s
- View all generated numbers at once
- Uses SIMD acceleration where applicable (ARM NEON, AVX/AVX2, or AVX-512F)

### Coming Soon!

- Double and floating point generation support
- Benchmarks
- More test results!

## INSTALLATION

ADAM was developed for macOS and Linux systems. It may be possible to run on other operating systems but I haven't checked or configured the program for other systems/distros. The macOS version makes use of ARM NEON Intrinsics, but the equivalent logic is implemented using AVX/AVX2 or AVX-512F. Either way, support for at least one of these sets of intrinsics is required. 

By default the executable is installed to `~/.local/bin` but you can change this by tweaking the `INSTALL_DIR` variable in the Makefile.

```
git clone https://github.com/pre-eth/adam.git
cd adam
make
adam -h
```

And you should be good to go! 

## SYNOPSIS

<pre>
adam [-h|-v|-l|-b] [-dx] [-w <em>width</em>] [-a <em>bit_multiplier</em>] [-r <em>results</em>]
     [-s <em>seed?</em>] [-n <em>nonce?</em>] [-u <em>amount?</em>]
</pre>

If you run `adam` with no arguments, you will get one randomly generated 64-bit number.

The following options are available:

    -h    Get all available options
    -v    Version of this software (X.Y.Z)
    -s    Get the seed for the generated buffer (no parameter) or provide your
          own. Seeds are reusable but should be kept secret.
    -n    Get the nonce for the generated buffer (no parameter) or provide your 
          own. Nonces should ALWAYS be unique and secret.
    -u    Generate a universally unique identifier (UUID). Optionally specify a 
          number of UUID's to generate (max 128)
    -r    Number of results to return (up to 256 u64, 512 u32, 1024 u16, or 2048 u8). No argument dumps     
          entire buffer
    -w    Desired size (u8, u16, u32, u64) of returned numbers (default width is u64)
    -d    Dump the whole buffer
    -b    Just bits...literally
    -a    Assess a binary or ASCII sample of 1000000 bits (1 Mb) written to a
          filename you provide. You can choose a multiplier within [1,8000]
    -l    Live stream of continuously generated numbers
    -x    Print numbers in hexadecimal format with leading prefix
    -o    Print numbers in octal format with leading prefix


## But is it REALLY secure?

“Proving” security is a very difficult thing to do, and a good deal of cryptanalysis is needed before domain wide recognition for the strength of any cryptographic algorithm is gained. That’s why the saying [“Don’t Roll Your Own Crypto”](https://security.stackexchange.com/questions/18197/why-shouldnt-we-roll-our-own) exists.

ADAM has passed the [NIST Test Suite for Random Bit Generation SP 800-22](https://csrc.nist.gov/publications/detail/sp/800-22/rev-1a/final) just like the original algorithm in the paper, along with other test suites mentioned below. However, conducting further testing is required before confident security guarantees can be made. This is just a toy RNG for now, and for production use I strongly recommend using something more thoroughly vetted by the field like [ChaCha20](https://datatracker.ietf.org/doc/html/rfc7539). 

While passing one or even all of these test suites doesn't guarantee that a RNG is cryptographically secure, it follows that a CSPRNG will pass these tests, so they nonetheless provide a measuring stick of sorts to reveal flaws in the design and various characteristics of the randomness of the generated numbers.

## TESTING

Testing is easy with the `-a` option. The minimum number of bits that can be outputted is 1 million bits (1Mb). You can provide a multiplier within [1, 8000] as mentioned above, and then write those bits to a text or binary file which can be supplied to any of the testing frameworks listed below, a different framework, or your own RNG tests!

For piping data directly into a testing program, use the `-b` option.

### SUMMARY

Click on a test to learn more.

| Status            | Name        | Description | 
| ------------------| ----------- | ----------- | 
| ✅ **PASS** | [ent](https://www.fourmilab.ch/random) | Calculates various values for a supplied pseudo random sequence like entropy, arithmetic mean, Monte Carlo value for pi, and more.
| ✅ **PASS** | [NIST STS](https://csrc.nist.gov/projects/random-bit-generation/documentation-and-software) | Set of statistical tests of randomness for generators intended for cryptographic applications. The general standard.
| ✅ **PASS** | [gjrand](https://gjrand.sourceforge.net) | Test suites for uniform bits, uniform floating point numbers, and normally distributed numbers, along with some other minor tests. Supposedly pretty tough!
| ✅ **PASS** | [Dieharder](https://webhome.phy.duke.edu/~rgb/General/dieharder.php) | Cleaned up version of George Marsaglia's DIEHARD suite plus the STS tests from above and RGB tests. Mostly of historical interest.
| ⏳ SOON | [TestU01](http://simul.iro.umontreal.ca/testu01/tu01.html) | 

TestU01 and PractRand are up next!

For details on how to run the tests above, please refer to `TESTING.md`.

## CONTRIBUTING

I started ADAM after looking at the source of ISAAC while I was learning C. I didn't intend to make anything groundbreaking or crazy, but I'm open to all cryptanalysis that can increase the cryptographic strength of this generator. Feel free to reach out to me or open an issue for any ideas/critiques/statistics you may have that can improve the implementation.

As far as code contributions though, I'd like to maintain the project myself for now as a continued learning opportunity. If I open the codebase up in the future I will amend the README, but as of now I'm not looking for other developers.
