# TESTING

Everything you need to know for testing ADAM with different popular RNG test suites.

## NIST

The STS can be downloaded from the link above. [This](https://www.slideshare.net/Muhammadhamid23/running-of-nist-test-109375052) is a good little quick start guide.

Results for testing 10Mb, 100Mb, 500Mb, and 1Gb are available in the `tests` subdirectory. 

Some results for the NonOverlappingTemplate tests may be borderline - this is expected since the output should be random, which means some runs will probability wise be weaker than others. The overall is what's most important, and to confirm for yourself you can run the Dieharder test suite which includes the STS tests and retries any `WEAK` results until a conclusive verdict can be reached.

## Dieharder

`adam -b | dieharder -a -Y 1 -k 2 -g 200`

Explanation of options can be found in Dieharder's [man page](https://linux.die.net/man/1/dieharder). I couldn't get the Dieharder binary working on macOS, so unfortunately this test may be limited to Linux users (if I'm wrong please let me know).

## gjrand

First download gjrand [here](https://gjrand.sourceforge.net), and then come back to this section. I tested on gjrand.4.3.0.0 so that's the version I'll proceed with. 

Using gjrand is easy but traversing it and figuring out what to build and how to build it is annoying so here's a quick little guide. I have only used the uniform randomness tests for binary integer output - the tests for binary doubles will be added once that functionality is implemented in ADAM.

### testunif

I recommend reading or skimming the `README.txt` file at the top level of this directory. It helps you understand the results.

On Unix systems building the binary should just work, so run:

```
cd gjrand.4.3.0.0/testunif/src
./compile
cd ..
adam -b | ./mcp --<SIZE_OPTION>
```

The size options are:

```
--tiny          10MB
--small         100MB
--standard      1GB
--big           10GB
--huge          100GB
--tera          1TB
--ten-tera      10TB
```

Results for sizes up to 10GB are available in the `tests` subdirectory.

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

