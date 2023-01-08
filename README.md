<pre style="text-align:center;">
<b style="color:@ADAM_TITLE@">
    █     ▀██▀▀█▄       █     ▀██    ██▀ 
   ███     ██   ██     ███     ███  ███  
  █  ██    ██    ██   █  ██    █▀█▄▄▀██  
 ▄▀▀▀▀█▄   ██    ██  ▄▀▀▀▀█▄   █ ▀█▀ ██  
▄█▄  ▄██▄ ▄██▄▄▄█▀  ▄█▄  ▄██▄ ▄█▄ █ ▄██▄ 
</b>                                      
v0.1.0
A SIMD accelerated cryptographically secure pseudo-random number generator (CSPRNG) inspired 
by the code of ISAAC, by Bob Jenkins. Added some of my own twists and turns to experiment. I 
did this just for fun and learning, not to set any records or make anything groundbreaking. 

<b style="color:@ADAM_BOLD@">Use at your own risk</b>. Criticism and suggestions are welcome.
</pre>                                   

This is a low level building block of my CLI tool suite, Preeth's
Kit. This is one of the tools it ships with upon install, and it
can also be incorporated into your own programs!

You can read more information about ISAAC [here](http://burtleburtle.net/bob/rand/isaacafa.html).

Check out the rest of Bob's site too, it's a treasure trove of 
interesting information!

# INSTALLATION

ADAM supports x86-64 based Linux systems. `gcc` version >=12.2.1
is required. It comes with `pk` so if you installed my toolkit,
then you're all set to go! In case you removed it or need to 
install it again, just do:

`pk -i adam`

And to run tests:

`pk -t adam`

ADAM was developed on Fedora, so it should work fine on RHEL flavors,
but as of now I have not been able to test on other distros.

ADAM begins its seeding process by using the Book of Genesis. When it's being 
built, each user's version of ADAM compiles a static lookup table that contains 
bytes from a different but uniform, identically constructed text sequence from 
the book. The bytes are packed into 624 64-bit integers.

The text sequence used is not the original biblical book, but a compilation 
that has been hashed by ISAAC! Over 20 iterations plus manual scrambling were 
used to compile a 600+ kB binary file of gibberish. This, of course, is an 
unacceptable size for a simple CLI utility to take up on your system, so a
random 5kB portion per user is used instead. The original file and generator
program are then promptly deleted.

# USAGE

# ALGORITHM

The name comes from the biblical figure Adam (a play on words since I 
came up with this while studying the source of ISAAC64). It is also a 
backronym like ISAAC that explains its process: 

**A**CCUMULATE 
**D**IFFUSE 
**A**UGMENT 
**M**ANGLE

## I. ACCUMULATE

The algorithm begins by querying the hardware for 2 TRNG numbers. The 
first random number is an index within `[0, TAU_BLOCKS - 1]` to 
retrieve a set of digits from the random assortment of tau digits in
`TAU[]`. 

The digit sequence is then XOR'd with a final TRNG'd number, and used
as our 64-bit nonce.

The next number is the start index to begin reading from in Genesis. The 
MAX_START value is currently 512 which is a power of 2, making modulus a 
matter of simple shifting. 112 bytes of information are arranged with the 
tau sequence and initialization vectors, with the resulting 128 64-bit
integers being planted in the `fruit` buffer. Precise indexing allows this
stage to benefit from autovectorization.

The true random numbers are gained from Intel's RDRAND instructions
directly, not `/dev/urandom`. This is one of the reasons why x86-64 
systems are currently required. These functions can stall if no random
data is available, so they are retried until a result is returned.

Even a difference of +1 or -1 in the range described above will produce ***very***
different output, as is expected of and the standard for any serious CSPRNG.

## II. DIFFUSE

While the data is stored in one buffer, ADAM operates on 8 tables of 
16 64-bit integers. To help you visualize the initial state of each block, 
here's a diagram:

The 8 IV's are derived from the following verse:

> ***"Be fruitful and multiply, and replenish the earth (Genesis 1:28)"***

(Which can also be considered a TL;DR for the whole algorithm in case you
don't want to keep reading.)

These 8 tables are, more scrupulously, our **S-Boxes**. Before they are used, 
the entire buffer must be permuted extensively. So for now, we will instead
operate on the buffer as if it was 16 columns of 8 elements.

A set of 8 initial addends are set to the Golden Ratio, per Bob's original 
implementation. Once they've been permuted themselves, this set of numbers is 
summed with the `fruit` buffer, with an additional permutation applied to them 
every 512 bits. Rows 1, 3, 5, and 7 swap positions with rows 2, 4, 6, and 8 
respectively, like so:

Finally, left hand columns are swapped with right hand columns such that column
1 is swapped with column 16, 2 with 15, and so on, working inwards like this:

## III. AUGMENT

This phase has 3 stages: **undulation**, **summation**, and **assimilation**. You can remember
it with the handy acronym [**USA**](https://media.tenor.com/gH-6XZCn-5EAAAAC/homer-simpson-usa-homer.gif)!

### UNDULATION

Consider this wave:


Since our working unit is only integers, suppose we roughly approximate the parameters
of this waveform and apply it to the buffer in the form of a compressive permutation.

This means that order must be changed and a certain subset of bits need to be selected. 

So to satisfy these criteria, we will do 3 things:

1.  Shift even columns up 2 elements, and odd columns down 2 elements
2.  Simulate a waveform `W` with amplitude `A:=4` and wavelength `L:=4` using the
    function `y = 4sin(4x)`, where `y` is the number of bits to shift left from `x` 
3.  Repeat Step 1

As the graphic shows, this function selectively compresses 64 of the elements. The
wavelength can be fine tuned and the polarity can be inverted as well if desired.

### SUMMATION

The summation stage begins with an expansive permutation. This is a simple 2 stage
form of cytokinesis.

We begin by returning to our initial configuration of 8 4x4 S-Boxes containing 16 
64-bit integers each. The 1st, 4th, 12th, and 16th element are responsible for the 
creation of 2 integers each, since they're used as addends in 2 separate operations:


This scheme is used to produce 80 new elements for 5 additional S-Boxes, which are
stored in the rest of the buffer. The remaining 3 boxes are synthesized from the 5
new ones, with the 5th box only producing 8 integers.

### ASSIMILATION

A longitudinal disruptive transposition is applied using alternating number sequences 
for the initial set `(0,5,4,X,1,6,2,X)` and new set `(4,X,1,6,2,X,3,7)`. The `X`
represents the indexes of elements that are replaced with empty bytes, and the two 
passwords are derived from the firing order of the Lamborghini Huracán's 5.2L V10 
engine. Here "password" is just the proper term; these are nothing-up-my-sleeve 
sequences.

The permutation logic of ISAAC64 is extended to 16 integers at a time, and used to
thoroughly blend the original set and its offspring like so:


We now have a rudimentary buffer of 256 randomly generated 64-bit integers, but we
still have some work to do before it's ready to use.

## IV. MANGLE

The last step makes use of the same quarter rounding functions used by the ChaCha
derived BLAKE2b hash function. These are also designed to operate on 16 blocks of 
64-bit integers.

2 rounds are applied per table, with the first one operating on columns and rows
like the original Salsa20 cipher, and the second operating vertically and diagonally
like ChaCha20. And with that, the user now has a pool of 256 cryptographically 
generated random numbers, which can be retrieved and used upon will! 

Bit shifting is used to satisfy arbitrary precision needs from the user. The number
is updated after the right shift, so the original isn't retained. Once a number has
been completely used, the buffer "regenerates" from the surrounding tables and fills
in the location with a new value.

# TESTS

# LICENSE: MIT

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