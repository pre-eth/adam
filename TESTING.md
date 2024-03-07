# TESTING

Everything you need to know for testing ADAM with different popular RNG test suites.

## adam -e 

This is a collection of miscellaneous heuristics implemented by me, just for logging some properties of some output sequence. It's nowhere near as important as the feedback from tried and true testing suites, but it's good for some quick information about the sequences you generate.

Additionally, the ENT framework is integrated into this collection, for a total of 13 pieces of information (plus initial state) that are returned to the user:

- monobit frequency
- any generated zeroes
- range distribution + chi-square
- max and min values
- parity: even and odd number totals
- chaotic seed distribution + chi-square
- runs: total # of runs and longest run (increasing/decreasing)
- average gap length
- entropy                     (ENT)
- chi-square                  (ENT)
- arithmetic mean             (ENT)
- monte carlo value for pi    (ENT)
- serial correlation          (ENT)

## NIST

The STS can be downloaded [here](https://csrc.nist.gov/projects/random-bit-generation/documentation-and-software). [This](https://www.slideshare.net/Muhammadhamid23/running-of-nist-test-109375052) is a good little quick start guide.

Results for testing 10Mb, 100Mb, 500Mb, and 1Gb are available in the `tests` subdirectory. 

Some results for the NonOverlappingTemplate tests may be borderline - this is expected since the output should be random, which means some runs will probability wise be weaker than others. The overall is what's most important, and to confirm for yourself you can run the Dieharder test suite which includes the STS tests and retries any `WEAK` results until a conclusive verdict can be reached.

NIST has created a [manual](https://nvlpubs.nist.gov/nistpubs/legacy/sp/nistspecialpublication800-22r1a.pdf) detailing each of the different tests, available for free download. 

## Dieharder

`adam -b | dieharder -a -Y 1 -k 2 -g 200`

Explanation of options can be found in Dieharder's [man page](https://linux.die.net/man/1/dieharder). I couldn't get the Dieharder binary working on macOS, so unfortunately this test may be limited to Linux users, unless you try using a Docker container (if I'm wrong please let me know).

## gjrand

First download gjrand [here](https://gjrand.sourceforge.net), and then come back to this section. I tested on gjrand.4.3.0.0 so that's the version I'll proceed with. 

Using gjrand is easy but traversing it and figuring out what to build and how to build it is annoying so here's a quick little guide. 

### testunif

I recommend reading or skimming the `README.txt` file at the top level of this directory. It helps you understand the results.

On Unix systems building the binary should just work, so run:

```
cd gjrand.4.3.0.0/testunif/src
./compile
cd ..
adam -b | ./mcp --<SIZE_OPTION>
```

where SIZE_OPTION is one of:

```
--tiny          10MB
--small         100MB
--standard      1GB
--big           10GB
--huge          100GB
--tera          1TB
--ten-tera      10TB
```

Results for sizes up to 100GB are available in the `tests` subdirectory.

### testfunif

I recommend reading or skimming the `README.txt` file at the top level of this directory.

To build and run:

```
cd gjrand.4.3.0.0/testfunif/src
./compile
cd ..
adam -f -a
File name: ftest
Output type (1 = ASCII, 2 = BINARY): 2
Sequence Size (x 1000): 1500
Scaling factor: 1
./mcpf --tiny < ftest
```

The size options are:

```
--tiny          1M doubles
--small         10M doubles
--standard      100M doubles (default)
--big           1G doubles
--huge          10G doubles
--even-huger    100G doubles
--tera          1T doubles.
```

Results for sizes up to 10G (10 billion) doubles are available in the `tests` subdirectory.

Finally, here is a summary of the meaning of gjrand's P-values.

```
The one-sided P-value is supposed to be a "single figure of merit" that
summarises a test result for people in a hurry. It doesn't usually encapsulate
all the information found by a particular test. Instead it usually focuses on
the most common or most serious failure modes. Some tests produce several
other results before the P-value, and these are worth reading and
understanding if you really want to know what's going on. In many cases the
actual tests are two-sided. But the P-value reported will always be one-sided.
Some are not very accurate, but should be close enough to support the rough
guide to interpretation below.

Interpretation of one-sided P-values.

P > 0.1 : This P-value is completely ok. Don't try to compare
	P-values in this range. "0.9 is higher than 0.4 so 0.9 must
	be better." Wrong. "0.5 is better than 0.999 because 0.5 is
	right in the middle of the range and therefore more random."
	Wrong. Each one-sided P-value test was designed to return an
	almost arbitrary value P > 0.1 if no bad behaviour was detected.
	If P > 0.1, you know no bad behaviour was detected, and you shouldn't
	try to interpret the result as anything more than that.
	Of course, a good P value like this doesn't prove the generator is
	good. It is always possible trouble will be spotted with a
	different test, or more data.

P <= 0.1 : There might be some cause for concern. In this range,
	the lower P is, the worse it is.

0.001 < P <= 0.1 : Not really proof that anything is wrong, but reason for
	vague suspicion. Further testing with a different random seed
	and perhaps larger data size is recommended. If a generator ALWAYS
	produces P values in this range or lower for a particular test,
	after many runs with different seeds, then it's probably broken.

1.0e-6 < P <= 0.001 : Like above, but moreso. Here you should start to worry.

1.0e-12 < P <= 1.0e-6 : There is a very remote possiblility that the
	generator is ok, but you should be seriously worried. After you
	see a P value this low, you can only rehabilitate the generator
	if you run the test that failed for many different seeds and
	it repeatedly produces good P-values with no more seriously bad ones.

P <= 1.0e-12 : You are most unlikely to see a good generator do this on a
	good test even once in your lifetime. You would be safe to reject any
	generator that does this on a properly calibrated test.
```

**Note:** For the next two test suites, I assume you've cloned the [testingRNG](https://github.com/lemire/testingRNG) repository.

## TestU01

To build from the root of `testingRNG`:

```
cd testu01
make
```

You might get a linker error when trying to install the examples, but just scroll up and make sure you see something like "Libraries have been installed in <PATH>". Now there should be a folder named `build` present.

Now, `cd` back to `build` in the ADAM directory:

```
cp ./libadam.a <TESTU01_PATH>/build/lib/libadam.a
cp ./adam.h <TESTU01_PATH>/build/include/adam.h
```

Where <TESTU01_PATH> is replaced with the path of the top-level TestU01 folder respectively on your system. Once you're done that, create a new C source file in `<TESTU01_PATH>/build` with the following:

```c
#include "include/TestU01.h"
#include "include/adam.h"

static adam_data data;

unsigned int adam_get(void)
{
    return adam_int(data, 32, 0);
}

int main()
{
    data = adam_setup(NULL, NULL);
    
    // Create TestU01 PRNG object for our generator
    unif01_Gen *gen = unif01_CreateExternGenBits("adam", adam_get);

    // Run the tests.
    bbattery_Crush(gen);
    
    // Clean up.
    unif01_DeleteExternGenBits(gen);
    adam_cleanup(data);

    return 0;
}
```

TestU01 works with 32-bit data so we need to set the width parameter accordingly. Now compile and run (swap `crush.c` with the name of your file):

```
gcc -std=c99 -Wall -O3 -o crush crush.c -Iinclude -Llib -ltestu01 -lprobdist -lmylib -lm -ladam -march=native
./crush
```

Crush should start running if you did everything correctly. It will take a while. You can swap out Crush with other batteries easily by changing the `bbattery_Crush` function call to `bbattery_SmallCrush`, `bbattery_Rabbit`, etc. For more details about the test suite, consult the [guide](http://simul.iro.umontreal.ca/testu01/guideshorttestu01.pdf), which is also available in the `doc` folder.

## PractRand

To build from the root of `testingRNG`:

```
cd practrand
make
```

Then, assuming `adam` is in your path (if not, copy it to the build folder): 

`adam -b / RNG_test stdin8`

PractRand also includes a wealth of great documentation about its test results and thought processes. See any of the text files with the `Test_` prefix to learn more about how the tests work, how the author determines their results and grading system, and the author's thoughts on the other test suites listed here.
