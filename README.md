<pre style="text-align:center;">
<p align="center">
    █     ▀██▀▀█▄       █     ▀██    ██▀ 
   ███     ██   ██     ███     ███  ███  
  █  ██    ██    ██   █  ██    █▀█▄▄▀██  
 ▄▀▀▀▀█▄   ██    ██  ▄▀▀▀▀█▄   █ ▀█▀ ██  
▄█▄  ▄██▄ ▄██▄▄▄█▀  ▄█▄  ▄██▄ ▄█▄ █ ▄██▄ 

v1.0.0

<b>Use at your own risk</b>. Criticism and suggestions are welcome.
</p>
</pre>         

ADAM is an actively developed cryptographically secure pseudorandom number generator (CSPRNG) inspired by ISAAC64. At the heart of the generator is an implementation of the algorithm described in [François M et al. Pseudo-random number generator based on mixing of three chaotic maps. Commun Nonlinear Sci Numer Simulat (2013)](https://doi.org/10.1016/j.cnsns.2013.08.032), which uses a chaotic function over multiple iterations to produce random bits with strong cryptographic properties. ADAM incorporates parts of ISAAC’s logic into this algorithm where it is applicable to form a compact number generation scheme that’s easy to use, tune, and test.

Also, just like ISAAC, ADAM is a backronym that describes its steps:

**A**ccumulate seed content in the buffer <br>
**D**iffuse the buffer with logic adapted from ISAAC <br>
**A**pply the necessary iterations of the chaotic function <br>
**M**ix the chaotic maps together to produce the output vector

You can find a deeper dive into the algorithm behind the number generation process in the paper above, and you can read more information about ISAAC and its wonderful author Bob Jenkins [here](http://burtleburtle.net/bob/rand/isaacafa.html).

## FEATURES

- Avoids brute force and differential attacks
- Only two (optional) input parameters: one 64-bit seed and one 64-bit nonce
- Space Complexity: O(N)
- Output sequence is irreversible
- Easy interface for bit generation in both ASCII and binary form. Output up to 1GB at a time.
- Alternatively, stream bits directly to the `stdin` of your own programs, RNG test suites, etc.
- Extract different precisions of numbers from the buffer
- Generate RFC 4122 compliant UUID’s
- View all generated numbers at once
- Reports execution time for number generation process
- Continuously stream and regenerate random numbers
- Uses SIMD acceleration where applicable (AVX2/AVX-512F)

### Coming Soon!
- Multithreading
- Better performance, including benchmarks and self-benchmark ability
- Examination option for exploring generated buffers based on ent and Bob Jenkins' own RNG tests!
- Some other cool surprises :)

## INSTALLATION

ADAM was developed on Fedora for x86 64-bit Linux systems. It may be possible to run on other operating systems but I haven't checked or configured the program for other systems/distros. AVX/AVX2 is required, AVX-512F is supported, but off by default; to enable it, you need to set the `AVX512` Makefile variable to `1` (assuming you have the proper CPUID flags).

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
adam [-h|-v|-l|-b] [-dx] [-p <em>precision</em>] [-a <em>bit_multiplier</em>] [-r <em>results</em>]
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
    -r    Number of results to return (up to 256 u64, 512 u32, 1024 u16, or 2048 u8)
    -p    Desired size (u8, u16, u32, u64) of returned numbers (default is u64)
    -d    Dump the whole buffer
    -b    Just bits...literally
    -a    Assess a binary or ASCII sample of 1000000 bits (1 MB) written to a
          filename you provide. You can choose a multiplier within [1,1000]
    -l    Live stream of continuously generated numbers
    -x    Print numbers in hexadecimal format with leading prefix


## But is it REALLY secure?

“Proving” security is a very difficult thing to do, and a good deal of cryptanalysis is needed before domain wide recognition for the security properties of any cryptographic algorithm is gained. That’s why the saying [“Don’t Roll Your Own Crypto”](https://security.stackexchange.com/questions/18197/why-shouldnt-we-roll-our-own) exists.

ADAM has passed the [NIST Test Suite for Random Bit Generation SP 800-22](https://csrc.nist.gov/publications/detail/sp/800-22/rev-1a/final) just like the original algorithm in the paper, but conducting further testing is required before strong security guarantees can be made. This is just a toy RNG for now, and for production use I strongly recommend using something more thoroughly vetted by the field like [ChaCha20](https://datatracker.ietf.org/doc/html/rfc7539). 

While passing one or even all of these test suites doesn't guarantee that a RNG is cryptographically secure, it follows that a CSPRNG will pass these tests, so they nonetheless provide a measuring stick of sorts to reveal flaws in the design and various characteristics of the randomness of the generated numbers.

## TESTING

Testing is easy with the `-a` option. The minimum number of bits that can be outputted is 1 million (1M) bits. You can supply a multiplier within [1, 1000] as mentioned above, and then write those bits to a text or binary file through the `-a` option that you can supply to any of the testing frameworks listed below, a different framework, or your own RNG tests!

### SUMMARY

Click on a test to learn more.

| Status      | Name        | Description | Status |
| ----------- | ----------- | ----------- | ------ |
| ⌛          | [chi.c](http://burtleburtle.net/bob/rand/testsfor.html) | From Bob Jenkins (author of ISAAC), calculates the distributions for the frequency, gap, and run tests exactly | PENDING
| ✅          | [NIST](https://csrc.nist.gov/projects/random-bit-generation/documentation-and-software) | Set of statistical tests of randomness for generators intended for cryptographic applications | **PASS**
| ⌛          | [ent](https://www.fourmilab.ch/random) | Calculates various values for a supplied pseudo random sequence like entropy, arithmetic mean, Monte Carlo value for pi, and more | PENDING
| ⌛          | [gjrand](https://gjrand.sourceforge.net) | Test suites for uniform bits and normally distributed numbers | PENDING
| ⌛          | [DIEHARDER](https://webhome.phy.duke.edu/~rgb/General/dieharder.php) | Cleaned up version of George Marsaglia's DIEHARD suite, with additional parameterizable tests | PENDING 

Files containing test results for 10M, 100M, 200M, 250M bits, 500M bits are available in the `tests` subdirectory with the appropriate prefix per test. 1B bits will be added once the 1 GB/s benchmark is passed.

## CONTRIBUTING

I started ADAM just for fun after looking at the source of ISAAC while I was learning C. I didn't intend to make anything groundbreaking or crazy, but I welcome all cryptanalysis that can increase the cryptographic strength of this generator. Feel free to reach out to me or open an issue for any ideas/critiques/statistics you may have that can improve the implementation.

As far as code contributions though, I'd like to maintain the project myself for now as a continued learning opportunity. If I open the codebase up in the future I will amend the README, but as of now I'm not looking for other developers.
