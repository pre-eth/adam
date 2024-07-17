# ADAM - ALGORITHM

This file illustrates what ADAM does visually and explains things in clear language so that even someone with no knowledge of random number generation will be able to follow along.

All analysis here will be updated as needed to include new insights and observations, and fleshed out with more detail as time goes on.

**LAST EDITED:** 2024-07-15

## <a id="quick"></a> Quickstart

**Quickstart** | 
[Internal State](#internal) | 
[Chaotic Function](#chaos) | 
[Initialize](#init) | 
[Accumulate](#acc) | 
[Diffuse](#diff) | 
[Apply](#apply) | 
[Mix](#mix) | 
[Generate](#gen)

### What is a CSPRNG?

A **CSPRNG (cryptographically secure pseudorandom number generator)** is a program that produces random binary data in order to fulfill some sort of purpose, which usually involves being consumed by some other type of application. The *pseudo*random part is important, because all PRNGs are **deterministic**. By utilizing one or more input parameters, the generator is set in a variety of states, and a deterministic generator always produces the same output for a given state. This ease of reproducibility is crucial and provides the user with reliability and control over the number generation process. 

Not all PRNGs provide cryptographic security. Usually, security must be verified through detailed cryptanalysis, mathematical and/or logical proofs, and the presence of no statistical defects. Additionally, the output must be totally indistinguishible from random noise, nothing about the output should enable attackers to extract the input information, and the only way to predict the output should be if and only if an adversary obtains the original input parameters. The entropy of the seed should not affect the quality of the random sequence. For all these reasons and several more, the designs of CSPRNGs are usually much more (and rightfully so) scrutinized before widespread recognition and production use.

### Why are random numbers important?

Random numbers are the raw materials of modern cryptography because they facilitate the encryption, decryption, transformation, hashing, and watermarking of information to ensure that the desired standard of security or obfuscation is met. They are a fundamental part of many different software offerings, from online banking and gambling systems to instant messengers and video games, not to mention all manners of cryptographic research.

### But you said **pseudo**random... so it's not *actually* random?

There are **TRNGs (true random number generators)** which are **non-deterministic** because they consistently reseed themselves by stochastically observing and recording a number of different physical processes (like temperature, thermal noise, fan speed, keyboard clicks, etc.). Since these physical phenomena can be highly unpredictable, they are fed to a **conditioning** stage, which is a feature of the TRNG that employs some type of algorithm to compile the recorded observations into a bit sequence.

The key word there is "*can be*" unpredictable. Since these sources of real-world entropy are difficult to reasonably forecast, it makes sense that they could fail to provide adequate entropy on their own, meaning it would take multiple different sources and/or a very good conditioning scheme to morph it into "apparently" random information. Additionally, since most of them measure physical processes, they are not portable and primarily implemented in hardware.

CSPRNGs and PRNGs, on the other hand, can be efficiently implemented in software, provide higher throughput and performance, and are seeded by simpler "plug 'n' play" type input parameters that enable reliable regeneration as needed. However, it's important to note that the output of a **good** CSPRNG, PRNG, or TRNG will all appear random to any test model or suite that is used to analyze their output sequences.

### So how do you measure "good"? How do we know if it's really random or secure?

If you want a practical answer, the honest answer is you can't. See [here](https://cs.stackexchange.com/a/67089) and [here](https://crypto.stackexchange.com/a/19680) for a brief overview of why.

But in addition to everything I mentioned in my first answer, there are some essential characteristics we can define to form some sort of standard of security that must be met before designating something as "cryptographically secure". For example, consider this excerpt from [this answer](https://crypto.stackexchange.com/a/88780):

> In cryptography, we define security properties by saying that there exists no adversary that can break it—that no program can solve a given problem related to it. In the case of
> cryptographic randomness, we say that:
> - There is no program that can, given the entire history of our random sequence, predict the next bit with better probability than the uniform distribution (i.e. better than 1/2).
> - The above remains true, even if the state of the RNG (but not its key) is leaked."
>
> The cryptographic primitives we herald as secure are "proven" in one of two ways:
>
> - They are published and the entire security community is invited to try to invent a program that breaks it.
> - They are proven to be unbreakable iff some other primitive they are based on is unbreakable.

### Can I test ADAM myself and see if it's everything it's cracked up to be?

Sure! I provide instructions on how to test ADAM, whether you want to use the tests that comes with the CLI or your own choice of external test suite in the file [TESTING.md](TESTING.md).

If you want to see the results already compiled from different popular RNG test suites, check the [tests](tests) directory. A summary table is also available in TESTING.md.

### What else do I need to know to understand this file?

I would hope that you have some familiarity with bitwise operations and the ability to read C code. Any cryptographic or statistics knowledge, especially regarding randomness, will help too of course.

### Where can I learn more about this stuff?

Here are the resources I used to help me as I built this project, along with some other popular recommendations and good starting points:

**RNGs** <br>
- [ISAAC](https://burtleburtle.net/bob/rand/isaacafa.html)
- [ChaCha](https://cr.yp.to/chacha.html)
- [Fortuna](https://www.schneier.com/wp-content/uploads/2015/12/fortuna.pdf)
- [Serpent](https://www.cl.cam.ac.uk/~rja14/serpent.html)
- [TRIVIUM Specifications](https://www.ecrypt.eu.org/stream/p3ciphers/trivium/trivium_p3.pdf)
- [Bernstein, Daniel. (2008). ChaCha, a variant of Salsa20.](https://cr.yp.to/chacha/chacha-20080128.pdf)
- [X. Fei, S. Zhang and W. Sun, "New Analysis on Security of Stream Cipher HC-256," 2013 International Conference on Computational and Information Sciences, Shiyang, China, 2013, pp. 1766-1769, doi: 10.1109/ICCIS.2013.462.](https://eprint.iacr.org/2004/092.pdf)
- [Anderson, Ross & Biham, Eli & Knudsen, Lars. (2000). Serpent: A Proposal for the Advanced Encryption Standard.](https://www.cl.cam.ac.uk/~rja14/Papers/serpent.pdf)

**CRYPTOGRAPHY & CHAOS THEORY** <br>
- [Chaos Theory](https://en.wikipedia.org/wiki/Chaos_theory)
- [Logistic Maps](https://en.wikipedia.org/wiki/Logistic_map)
- [Tent Maps](https://en.wikipedia.org/wiki/Tent_map)
- [Lyapunov Characteristic Exponents](https://mathworld.wolfram.com/LyapunovCharacteristicExponent.html)
- [Donald E. Knuth - The Art of Computer Programming: Seminumerical Algorithms, Volume 2 (3rd Edition)](https://www.amazon.com/Art-Computer-Programming-Seminumerical-Algorithms/dp/0201896842)
- [Niels Ferguson, Bruce Schneier, and Tadayoshi Kohno. 2010. Cryptography Engineering: Design Principles and Practical Applications. Wiley Publishing.](https://www.amazon.com/exec/obidos/ASIN/0470474246/acmorg-20)
- [François M et al. Pseudo-random number generator based on mixing of three chaotic maps. Commun Nonlinear Sci Numer Simulat (2013)](https://doi.org/10.1016/j.cnsns.2013.08.032)
- [Michael François, David Defour, Christophe Negre. A Fast Chaos-Based Pseudo-Random Bit Generator Using Binary64 Floating-Point Arithmetic. Informatica, 2014, 38 (3), pp.115-124.](https://hal.science/hal-01024689)
- [Thomas W. Cusick and Pantelimon Stănică (Auth.) - Cryptographic Boolean Functions and Applications (2017, Academic Press)](https://shop.elsevier.com/books/cryptographic-boolean-functions-and-applications/cusick/978-0-12-811129-1)
- [Sulak, Fatih. Statistical Analysis of Block Ciphers and Hash Functions. Middle East Technical University, 0 2011.](http://etd.lib.metu.edu.tr/upload/12613045/index.pdf)

**STATISTICS CONCEPTS** <br>
- [Null Hypothesis](https://resources.nu.edu/statsresources/hypothesis)
- [Chi-square](https://www.bmj.com/about-bmj/resources-readers/publications/statistics-square-one/8-chi-squared-tests)
- [Chi-square Table up to 1000](https://www.medcalc.org/manual/chi-square-table.php)
- [Z-score](https://www.cwu.edu/academics/academic-resources/learning-commons/_documents/z-score.pdf)
- [P-values](https://www.scribbr.com/statistics/p-value/)
- [More about p-values](https://researcher.life/blog/article/what-is-p-value-calculation-statistical-significance/)
- [P-value from chi-square](https://stats.stackexchange.com/a/25107)
- [P-value from z-score](https://www.wallstreetmojo.com/p-value-formula/)
- [Incomplete Gamma Function](https://mathworld.wolfram.com/IncompleteGammaFunction.html)
- [Gamma Function](https://mathworld.wolfram.com/GammaFunction.html)
- [Fisher's Method](https://en.wikipedia.org/wiki/Fisher%27s_method)
- [Stouffer's Z-Score method](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3135688/)

**RANDOMNESS TESTS** <br>
- [ENT](https://www.fourmilab.ch/random/)
- [DIEHARD](https://github.com/GINARTeam/Diehard-statistical-test?tab=readme-ov-file)
- [gjrand](https://gjrand.sourceforge.net/)
- [CACert](https://www.cacert.at/cgi-bin/rngresults)
- [PractRand](https://pracrand.sourceforge.net/)
- [NIST Suite Slideshow Explanation](https://csrc.nist.gov/CSRC/media/Projects/Random-Bit-Generation/documents/ANSIX9F1.pdf)
- [Bassham, L. , Rukhin, A. , Soto, J. , Nechvatal, J. , Smid, M. , Leigh, S. , Levenson, M. , Vangel, M. , Heckert, N. and Banks, D. (2010), A Statistical Test Suite for Random and Pseudorandom Number Generators for Cryptographic Applications, Special Publication (NIST SP), National Institute of Standards and Technology, Gaithersburg, MD, [online], https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=906762](https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=906762)
- [Pierre L'Ecuyer and Richard Simard. 2007. TestU01: A C library for empirical testing of random number generators. ACM Trans. Math. Softw. 33, 4, Article 22 (August 2007), 40 pages. https://doi.org/10.1145/1268776.1268777](https://doi.org/10.1145/1268776.1268777)
- [F. Sulak, “A New Statistical Randomness Test: Saturation Point Test”, IJISS, vol. 2, no. 3, pp. 81–85, 2013.](https://dergipark.org.tr/en/pub/ijiss/issue/16060/167857)
- [Maurer, U.M. A universal statistical test for random bit generators. J. Cryptology 5, 89–105 (1992). https://doi.org/10.1007/BF00193563](https://link.springer.com/article/10.1007/BF00193563)
- [Alcover, Pedro & Guillamón, Antonio & Ruiz, M.D.C.. (2013). A New Randomness Test for Bit Sequences. Informatica (Netherlands). 24. 339-356. 10.15388/Informatica.2013.399.](https://www.researchgate.net/publication/288404484_A_New_Randomness_Test_for_Bit_Sequences)
- [John von Neumann. "Distribution of the Ratio of the Mean Square Successive Difference to the Variance." Ann. Math. Statist. 12 (4) 367 - 395, December, 1941. https://doi.org/10.1214/aoms/1177731677](https://projecteuclid.org/journals/annals-of-mathematical-statistics/volume-12/issue-4/Distribution-of-the-Ratio-of-the-Mean-Square-Successive-Difference/10.1214/aoms/1177731677.full?tab=ArticleFirstPage)
- [Hernandez-Castro, Julio & Sierra, José & Seznec, Andre & Izquierdo, Antonio & Ribagorda, Arturo. (2005). The strict avalanche criterion randomness test. Mathematics and Computers in Simulation. 68. 1-7. 10.1016/j.matcom.2004.09.001.](https://www.researchgate.net/publication/222525312_The_strict_avalanche_criterion_randomness_test)
- [Oprina, Andrei-George et al. “WALSH-HADAMARD RANDOMNESS TEST AND NEW METHODS OF TEST RESULTS INTEGRATION.” (2009).](https://www.semanticscholar.org/paper/WALSH-HADAMARD-RANDOMNESS-TEST-AND-NEW-METHODS-OF-Oprina-Popescu/42de0c0c663461bfded8e5b29171e40f34ffed85)

If you feel comfortable moving on, let's begin by discussing ADAM's internal state.

## <a id="internal"></a> Internal State

[Quickstart](#quick) | 
**Internal State** | 
[Chaotic Function](#chaos) | 
[Initialize](#init) | 
[Accumulate](#acc) | 
[Diffuse](#diff) | 
[Apply](#apply) | 
[Mix](#mix) | 
[Generate](#gen)

The following variables are the parameters that maintain internal state in ADAM and control the number generation process.

![INTERNAL STATE](https://github.com/user-attachments/assets/4629f5a3-0110-47ed-8234-3f02e27e3fdb) <br/>

Where `u8` is `uint8_t`, `u64` is `uint64_t`, and `f64` is `double`. The bracket notation is used to indicate an array. This is defined in [api.c](src/api.c) as:

```c
struct adam_data_s {
    // 256-bit seed
    u64 seed[4];

    // 96-bit nonce
    u32 nonce[3];

    // Chaotic work buffer - 256 64-bit integers = 2048 bytes
    u64 out[BUF_SIZE] ALIGN(ADAM_ALIGNMENT);

    // The seeds supplied to each iteration of the chaotic function
    double chseeds[ROUNDS] ALIGN(ADAM_ALIGNMENT);

    // Current index in buffer
    u8 buff_idx;
};

// Opaque pointer to hide internal members from user
// Closest we can get to "private" fields in C :P
typedef struct adam_data_s * adam_data;
```

Let's look at some properties of ADAM as a result of this internal state configuration.

### ADAM has a statespace of 2¹⁷²⁵⁶

If internal state has `M` bits, total state space is `pow(2, M)` values. 

| State Member | Bits |
| ------------ | ---- |
| Seed | 256 | 
| Nonce | 96 |
| Output Index | 8 |
| Output Buffer | 16384 | 
| Chaotic Seeds | 512 | 
| Total | **17256** |

### ADAM has futureproof minimum and average periods

Pseudorandom number generators are deterministic and finite, since their scope is bound by the size of their statespace. This means that after producing a certain amount of values, they will have "cycled" back to a previous internal state, and the output stream will begin to repeat itself. The **period** (also called cycle or cycle length) is the number of values an RNG can theoretically produce before its output restarts. The period is derived from the construction of the generator itself so it cannot be arbitrarily set, and furthermore it is an **upper bound**. Technically, the real cycle length before the RNG is compromised may be lower, however in practice the most important thing is to make sure that observations of the RNG's minimum cycle length show that it is "long enough" for its intended purposes.

Because ADAM does not reseed itself, and it evolves in a highly chaotic manner, it is difficult to draw definite conclusions about its period. But some observations can be noted.

- **Minimum Period**: If we assume that ADAM does indeed act as a random permutation after its setup stage, that would mean all 2¹⁷²⁵⁶ possible cycle lengths are equally likely. This means that the probability of a minimum period less than or equal to 2ⁿ is `2ⁿ / 2¹⁷²⁵⁶ = 2ⁿ⁻¹⁷²⁵⁶`. Using this formula, it is clearly evident that the probability of a minimum period exponentially decreases based on how small/insecure it may be. For example, any period less than say 2²⁵⁶ has a probability of `2²⁵⁶⁻¹⁷²⁵⁶ = 2⁻¹⁷⁰⁰⁰`.

- **Average Period**: Simple math tells us that the average cycle length assuming all are equally likely would be `2¹⁷²⁵⁶ / 2 = 2¹⁷²⁵⁶⁻¹ = 2¹⁷²⁵⁵`. 

### ADAM is multi-cyclic

If the statespace of an RNG is larger than its minimum period, it is termed **multi-cyclic** because there are multiple possible cycles that the RNG could take based on its input parameters. Thus, the 2 proofs above clearly demonstrate that ADAM is multi-cyclic.

## <a id="chaos"></a> Chaotic Function

[Quickstart](#quick) | 
[Internal State](#internal) | 
**Chaotic Function** | 
[Initialize](#init) | 
[Accumulate](#acc) | 
[Diffuse](#diff) | 
[Apply](#apply) | 
[Mix](#mix) | 
[Generate](#gen)

![LOGISTIC FUNCTION](https://github.com/user-attachments/assets/74b6609a-a512-44e3-8bfe-9a3d40d664dd) <br/>

This logistic function is used to produce our **chaotic buffer**, which we perform operations on to generate random bits while replacing multiple elements in the buffer as we progress. It is a function that has been referenced in several cryptographic papers regarding chaos theory, and used to create various pseudorandom number generation schemes. However, these algorithms are rarely given serious attention from most of the cryptographic community due to readily available, tried and tested standards such as AES and ChaCha. Additionally, while most of these chaotic RNG algorithms theoretically may provide good results, implementing them in software (especially the naive, "apparent" way) is a pain and often results in poor performance.

Where ADAM differs from most of these algorithms is that the chaotic function is only used to generate an internal buffer, and the random bits that are outputted come from simple operations that are performed on the buffer! However, ADAM still retains the tradeoff that commonly arises when implementing chaotic algorithms in general: the use of floating point operations (FLOPs).

While FLOPs are a common requirement when programming chaotic systems, incorporating any kind of FLOPs is less than ideal when creating a RNG. You'll find with little effort that any widely employed RNG algorithm relies exclusively on integer operations. But ADAM doesn't suffer from using FLOPs because SIMD intrinsics are a crucial component of the ADAM implementation and a primary reason for its high throughput, even when using the chaotic function. After the chaotic buffer is created though, SIMD isn't necessary at all for the basic addition, subtraction, XOR, and rotation operations used to produce our random bits. On modern computers, these operations are so efficient that ADAM can still use the chaotic function to update its internal state!

Since we are using SIMD (AVX2/AVX512-F on x86, ARM NEON v8-a on aarch64), we process blocks of 8 64-bit integers or 8 doubles at a time. The chaotic function requires a seed for the initial value of `X` between `(0.0, 0.5)`, but because of our block size, we need to maintain 8 of them.

```c
// Number of rounds we perform at a time
#define ROUNDS              8
```

We use the seeds to initialize the chaotic function, and once the system is set in motion, we use the new values that it generates to permute, mix, and replace our internal buffer before it's ready for use. And thanks to our use of SIMD intrinsics, we can perform 8 chaotic function rounds at a time to speed things up and allow us to write 512 bits at a time to the chaotic buffer. But this means we need to align the arrays before they can be used for SIMD, so ADAM defines a macro to help with that.

```c
#if defined(__AARCH64_SIMD__) || defined(__AVX512F__)
    #define ADAM_ALIGNMENT      64
#else
    #define ADAM_ALIGNMENT      32
#endif
```

If you look in [rng.h](include/rng.h), you'll see the 6 main primitives of the ADAM algorithm. The implementations are all in [rng.c](src/rng.c). We're going to take a look at each one.

## <a id="init"></a> Initialize

[Quickstart](#quick) | 
[Internal State](#internal) | 
[Chaotic Function](#chaos) | 
**Initialize** | 
[Accumulate](#acc) | 
[Diffuse](#diff) | 
[Apply](#apply) | 
[Mix](#mix) | 
[Generate](#gen)

```c
void initialize(const u64 *restrict seed, const u64 nonce, u64 *restrict out, u64 *restrict mix);
```

Let's start from the beginning and see how we transform the input parameters (256-bit seed and 64-bit nonce) into an initial form of our internal state.

```c
/*
    8 64-bit IV's that correspond to the verse:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"

    Mix IV's with different configurations of seed values
*/
out[0] = 0x4265206672756974 ^ seed[0];
out[1] = 0x66756C20616E6420 ^ ((seed[1] << (nonce & 63)) | (seed[3] >> (64 - (nonce & 63))));
out[2] = 0x6D756C7469706C79 ^ seed[1];
out[3] = 0x2C20616E64207265 ^ ((seed[0] << 32) | (seed[2] >> 32));
out[4] = 0x706C656E69736820 ^ seed[2];
out[5] = 0x7468652065617274 ^ ((seed[2] << (nonce & 63)) | (seed[0] >> (64 - (nonce & 63))));
out[6] = 0x68202847656E6573 ^ seed[3];
out[7] = 0x697320313A323829 ^ ((seed[3] << 32) | (seed[1] >> 32));

// Initialize intermediate chaotic mix
mix[0] = ~out[4] ^ nonce;
mix[1] = ~out[5] ^ (nonce + 1);
mix[2] = ~out[6] ^ (nonce + 2);
mix[3] = ~out[7] ^ (nonce + 3);
mix[4] = ~out[0] ^ (nonce + 4);
mix[5] = ~out[1] ^ (nonce + 5);
mix[6] = ~out[2] ^ (nonce + 6);
mix[7] = ~out[3] ^ (nonce + 7);
```

We use the lower 64 bits of the nonce here.

`out` refers to our internal buffer. We use the first 8 indices to store our initialization vectors (IVs). The values in these indices are later overwritten entirely in `diffuse()` with different data.

`mix` refers to our "intermediate chaotic mix", which is used to help generate our set of chaotic seeds. We declare a temporary array to meet this end just like we did for the chaotic seeds, and then pass it into our primitives.

```c
// Intermediate chaotic mix - WORD_SIZE * 8 = 64 bytes
u64 mix_arr[ROUNDS] ALIGN(ADAM_ALIGNMENT);
```

While this scheme may look rudimentary, it is effective. You can supply seeds and nonces with off-balance or very low Hamming weights (even just 0s for both!) and ADAM's output quality will not observably falter at all, even with sequences as small as 1MB. You wouldn't be able to tell the difference between someone who used a good or bad seed, which makes it much more difficult for an attacker to try and crack the seed and/or nonce. Also, because of the deep interdependence in their usage, having one by itself is not enough to extract any of the RNG's internal state or draw firm conclusions about it from the output sequence.

## <a id="acc"></a> Accumulate

[Quickstart](#quick) | 
[Internal State](#internal) | 
[Chaotic Function](#chaos) | 
[Initialize](#init) | 
**Accumulate** | 
[Diffuse](#diff) | 
[Apply](#apply) | 
[Mix](#mix) | 
[Generate](#gen)

```c
void accumulate(u64 *restrict out, u64 *restrict mix, double *restrict chseeds);
```

It's at this stage that we introduce a macro from an existing CSPRNG: [ISAAC64](https://burtleburtle.net/bob/rand/isaacafa.html). This is a very important macro in ADAM as its the primary way we:
1. Prepare our intermediate chaotic `mix` so it can be used to generate our chaotic seed set.
2. Populate the chaotic buffer before we `apply()` the chaotic function and modify the contents.

```c
// For diffusion - from https://burtleburtle.net/bob/c/isaac64.c
#define ISAAC_MIX(a,b,c,d,e,f,g,h) { \
    a-=e; f^=h>>9;  h+=a; \
    b-=f; g^=a<<9;  a+=b; \
    c-=g; h^=b>>23; b+=c; \
    d-=h; a^=c<<15; c+=d; \
    e-=a; b^=d>>14; d+=e; \
    f-=b; c^=e<<20; e+=f; \
    g-=c; d^=f>>17; f+=g; \
    h-=d; e^=g<<14; g+=h; \
}
```

So we begin by mixing our initial state by iteratively applying this macro similar to its use in the `randinit()` function in [isaac64.c](https://burtleburtle.net/bob/c/isaac64.c).

```c
register u8 i = 0;

// Scramble
for (; i < ROUNDS; ++i) {
    ISAAC_MIX(mix[0], mix[1], mix[2], mix[3], mix[4], mix[5], mix[6], mix[7]);
}
```

If you've looked at ADAM's source, you might notice after this point is a mine field of SIMD macros. They aren't very intuitive to understand at first glance, I'll admit. So I'm going to rewrite the logic in simple C code similar to the code shown in `initialize()`. Doing this will make it much easier to see how we obtain our chaotic seeds.

First, we need to define another macro to help us stay within that `(0.0, 0.5)` limit I mentioned earlier.

```c
// To approximate (D / (double) __UINT64_MAX__) * 0.5 for a random int casted to double D
#define RANGE_LIMIT           2.7105054E-20
```

And now the simplified code. Per iteration, we do the following.

```c
// XOR the state in out with our chaotic mix
u64 a = out[0] ^ mix[0];
u64 b = out[1] ^ mix[1];
u64 c = out[2] ^ mix[2];
u64 d = out[3] ^ mix[3];
u64 e = out[4] ^ mix[4];
u64 f = out[5] ^ mix[5];
u64 g = out[6] ^ mix[6];
u64 h = out[7] ^ mix[7];

// Rotate results right by 32
a = (a >> 32) | (a << 32);
b = (b >> 32) | (b << 32);
c = (c >> 32) | (c << 32);
d = (d >> 32) | (d << 32);
e = (e >> 32) | (e << 32);
f = (f >> 32) | (f << 32);
g = (g >> 32) | (g << 32);
h = (h >> 32) | (h << 32);

// Cast to double and multiply by RANGE_LIMIT
// This gives us a double within (0.0, 0.5)
chseeds[0] = (double) a * RANGE_LIMIT;
chseeds[1] = (double) b * RANGE_LIMIT;
chseeds[2] = (double) c * RANGE_LIMIT;
chseeds[3] = (double) d * RANGE_LIMIT;
chseeds[4] = (double) e * RANGE_LIMIT;
chseeds[5] = (double) f * RANGE_LIMIT;
chseeds[6] = (double) g * RANGE_LIMIT;
chseeds[7] = (double) h * RANGE_LIMIT;

// Update state contents by adding results from XOR and rotation.
out[0] += a;
out[1] += b;
out[2] += c;
out[3] += d;
out[4] += e;
out[5] += f;
out[6] += g;
out[7] += h;
```

We do this for `ROUNDS` iterations before storing the final 8 chaotic seed values at the end of the loop into our `chseeds` struct member. Now we have obtained the set of initial seeds that we need to use the chaotic function!

Here's a guide that explains what's done here visually.

![ACCUMULATE](https://github.com/user-attachments/assets/e8ba50e3-1e15-4042-8bd5-dd5724fe2c28) <br/>

## <a id="diff"></a> Diffuse

[Quickstart](#quick) | 
[Internal State](#internal) | 
[Chaotic Function](#chaos) | 
[Initialize](#init) | 
[Accumulate](#acc) | 
**Diffuse** | 
[Apply](#apply) | 
[Mix](#mix) | 
[Generate](#gen)

```c
void diffuse(u64 *restrict out, u64 *restrict mix, const u64 nonce);
```

So we have our chaotic seeds. Next, we fill our internal buffer `out` with initial integer data using logic from an inner loop in the `randinit()` function from ISAAC64 mentioned earlier. In fact, `diffuse()` doesn't require any SIMD at all. We still process 512-bits at a time though, and use the ISAAC_MIX macro on our intermediate chaotic `mix` 32 times before this function completes, significantly reducing the correlation between its use now versus later in the `mix()` function. Also notice how the nonce comes back into play here to help us consistently add entropy before each use of the macro.

This is what we do each iteration of the loop in `diffuse()`. We use the high 64 bits of the nonce here.

```c
mix[0] += nonce;
mix[1] += nonce;
mix[2] += nonce;
mix[3] += nonce;
mix[4] += nonce;
mix[5] += nonce;
mix[6] += nonce;
mix[7] += nonce;

ISAAC_MIX(mix[0], mix[1], mix[2], mix[3], mix[4], mix[5], mix[6], mix[7]);

out[i + 0] = mix[0];
out[i + 1] = mix[1];
out[i + 2] = mix[2];
out[i + 3] = mix[3];
out[i + 4] = mix[4];
out[i + 5] = mix[5];
out[i + 6] = mix[6];
out[i + 7] = mix[7];
```

## <a id="apply"></a> Apply

[Quickstart](#quick) | 
[Internal State](#internal) | 
[Chaotic Function](#chaos) | 
[Initialize](#init) | 
[Accumulate](#acc) | 
[Diffuse](#diff) | 
**Apply** | 
[Mix](#mix) | 
[Generate](#gen)

```c
void apply(u64 *restrict out, const double *restrict chseeds);
```

Now we get to finally use the chaotic function! We enter the chaotic system for 32 iterations and produce 8 64-bit values per iteration. In each iteration, we also produce 8 new chaotic seed values to use in the next iteration when we make the next 8 calls to the chaotic function. This is an important detail: reusing chaotic seeds would have disastrous consequences for the output. The output of the chaotic function in one iteration MUST be the same value that is used for its input in the following iteration. 

This section reverts back to using SIMD operations. I will do what I did for `accumulate()` and provide the scalar version of the code along with a visual.

First, since this is a special constant, let's define a macro.

```c
// For the logistic function: 3.9999 * X * (1 - X)
#define COEFFICIENT           3.9999
```

Here's the simplified C code. Per iteration, we do the following.

```c
// 3.9999 * X * (1 - X) for all chaotic seeds
chseeds[0] = COEFFICIENT * chseeds[0] * (1.0 - chseeds[0]);
chseeds[1] = COEFFICIENT * chseeds[1] * (1.0 - chseeds[1]);
chseeds[2] = COEFFICIENT * chseeds[2] * (1.0 - chseeds[2]);
chseeds[3] = COEFFICIENT * chseeds[3] * (1.0 - chseeds[3]);
chseeds[4] = COEFFICIENT * chseeds[4] * (1.0 - chseeds[4]);
chseeds[5] = COEFFICIENT * chseeds[5] * (1.0 - chseeds[5]);
chseeds[6] = COEFFICIENT * chseeds[6] * (1.0 - chseeds[6]);
chseeds[7] = COEFFICIENT * chseeds[7] * (1.0 - chseeds[7]);

// Reinterpret chaotic seeds as binary floating point
// Add the mantissa values to the next 8 values in the buffer
out[i + 0] += (*((u64 *) &chseeds[0]) & 0xFFFFFFFFFFFFF);
out[i + 1] += (*((u64 *) &chseeds[1]) & 0xFFFFFFFFFFFFF);
out[i + 2] += (*((u64 *) &chseeds[2]) & 0xFFFFFFFFFFFFF);
out[i + 3] += (*((u64 *) &chseeds[3]) & 0xFFFFFFFFFFFFF);
out[i + 4] += (*((u64 *) &chseeds[4]) & 0xFFFFFFFFFFFFF);
out[i + 5] += (*((u64 *) &chseeds[5]) & 0xFFFFFFFFFFFFF);
out[i + 6] += (*((u64 *) &chseeds[6]) & 0xFFFFFFFFFFFFF);
out[i + 7] += (*((u64 *) &chseeds[7]) & 0xFFFFFFFFFFFFF);
```

We do this for 32 iterations. We use the chaotic function as another highly unpredictable source of entropy as we continue morphing our internal buffer. By using the ISAAC_MIX macro, we have already nicely expanded our initial state into an initial buffer. So we don't need to completely *replace* the contents, moreso just carry on flipping irregular amounts of bits to reduce any correlation with the input parameters. To meet this end, we provide a delta for adding to the value using the chaotic result. This delta comes from the result of our chaotic function!

Here's a guide that explains what's done here visually.

![APPLY](https://github.com/user-attachments/assets/b1544d22-8ba0-477c-8457-4ae383afed93) <br/>

## <a id="mix"></a> Mix

[Quickstart](#quick) | 
[Internal State](#internal) | 
[Chaotic Function](#chaos) | 
[Initialize](#init) | 
[Accumulate](#acc) | 
[Diffuse](#diff) | 
[Apply](#apply) | 
**Mix** | 
[Generate](#gen)

```c
void mix(u64 *restrict out, const u64 *restrict mix);
```

The final step we must do before we can start generating random bits for consumption by other sources is thoroughly mix the contents in our internal buffer. We do one more clean pass through the buffer before we move on to `generate()`, which is the only primitive called after the completion of `mix()`.

This final section also uses SIMD operations. Here is the scalar equivalent C code.

```c
// XOR current buffer contents with our chaotic mix
u64 a = out[i + 0] ^ mix[0];
u64 b = out[i + 1] ^ mix[1];
u64 c = out[i + 2] ^ mix[2];
u64 d = out[i + 3] ^ mix[3];
u64 e = out[i + 4] ^ mix[4];
u64 f = out[i + 5] ^ mix[5];
u64 g = out[i + 6] ^ mix[6];
u64 h = out[i + 7] ^ mix[7];

// Rotate results right by 32
a = (a >> 32) | (a << 32);
b = (b >> 32) | (b << 32);
c = (c >> 32) | (c << 32);
d = (d >> 32) | (d << 32);
e = (e >> 32) | (e << 32);
f = (f >> 32) | (f << 32);
g = (g >> 32) | (g << 32);
h = (h >> 32) | (h << 32);

// Add current buffer values to mix
mix[0] += out[i + 0];
mix[1] += out[i + 1];
mix[2] += out[i + 2];
mix[3] += out[i + 3];
mix[4] += out[i + 4];
mix[5] += out[i + 5];
mix[6] += out[i + 6];
mix[7] += out[i + 7];

// Perform three way XOR operation on the mix, buffer, and temp values
// Store results in buffer
out[i + 0] ^= mix[0] ^ a;
out[i + 1] ^= mix[1] ^ b;
out[i + 2] ^= mix[2] ^ c;
out[i + 3] ^= mix[3] ^ d;
out[i + 4] ^= mix[4] ^ e;
out[i + 5] ^= mix[5] ^ f;
out[i + 6] ^= mix[6] ^ g;
out[i + 7] ^= mix[7] ^ h;
```

And here's a guide that explains what's done here visually.

![MIX](https://github.com/user-attachments/assets/3a7e0f33-e27e-4dff-980c-2e15017e371c) <br/>

## <a id="gen"></a> Generate

[Quickstart](#quick) | 
[Internal State](#internal) | 
[Chaotic Function](#chaos) | 
[Initialize](#init) | 
[Accumulate](#acc) | 
[Diffuse](#diff) | 
[Apply](#apply) | 
[Mix](#mix) | 
**Generate**

```c
u64 generate(u64 *restrict out, u8 *restrict idx, double *restrict chseeds)
```

And with that, our logic for preparing our internal state is complete. Now we can output random bits!

We need to define some more macros so we can implement `generate()`. 

```c
/*
    CHFUNCTION is used for updating internal state using the output of the
    chaotic function

    CHMANT32 is for extracting the lower 32 bits of the chaotic value's
    binary representation. 

    Must take the mod 7 of x because it will be the data->buff_idx member
*/
#define CHFUNCTION(x, ch)       COEFFICIENT * ch[x & 7] * (1.0 - ch[x & 7])
#define CHMANT32(x, ch)         (*((u64 *) &ch[x & 7]) & __UINT32_MAX__)
```

Here is the simple algorithm we use to create our random integers.

```c
// Update value with chaotic function
chseeds[*idx & 7] = CHFUNCTION(*idx, chseeds);

// Get lower 32-bits of binary output and and shift left
const u64 m = (u64) CHMANT32(*idx, chseeds) << 32;

// Update value again with chaotic function
chseeds[*idx & 7] = CHFUNCTION(*idx, chseeds);

// Get current element
const u64 elem = out[*idx];

// Find two random indices within the top and bottom halves
const u8 a = *idx + 1 + (elem & 0x7F);
const u8 b = *idx + 128 + (elem & 0x7F);

// OR the lower 32 bits of this binary output onto <m>
// Form our output number by XORing <out[a]> with new <m> and <out[b]>
const u64 num = out[a] ^ out[b] ^ (m | CHMANT32(*idx, chseeds));

// Three-way XOR to replace current value
out[*idx] ^= out[a] ^ out[b];

// Increment our index (wraps around automatically from 0-255)
*idx += 1;

// Finish and return our randomly generated u64!
return num;
```

A random buffer index is chosen and stored prior to the `mix()` step. This function progresses through the internal buffer beginning at that index, and XORs two values selected via modulo from different halves of the `out` buffer with a temporary chaotic quantity to produce our output. The current value at `idx` is mutated via a three-way XOR operation with these other two values, and then `idx` is incremented. When each value in `out` is revisited, it will be a completely different value.

Because the computed indices `a` and `b` may repeat across different iterations, each value in the buffer can affect the output multiple times per revolution before they are replaced, and the modulo lookup scheme adds some confusion to prevent the observation of any fixed patterns. In addition, each value in `chseeds` is updated twice per call as we continue to make use of the logistic function as an additional entropy source for generating results. 
