/*
  Apply various randomness tests to a stream of bytes. Adapted to
  work comfortably with ADAM but no test logic has been modified

  Designed and implemented by John "Random" Walker in May 1985.

  For additional information and the latest version,
  see https://www.fourmilab.ch/random/

  Chi Square computation code was developed by Gary Perlman of the Wang
  Institute (full citations below) and has been minimally
  modified for use in this program.
*/
#include <math.h>

#include "../include/adam.h"
#include "../include/ent.h"

#define LOG2(x) (log2of10 * log10(x))
#define ex(x) (((x) < -BIGX) ? 0.0 : exp(x))

static u64 ccount[BUF_SIZE] ALIGN(64), // Bins to count occurrences of values
    totalc; // Total bytes counted

static double prob[BUF_SIZE]; // Probabilities per bin for entropy

static u8 mp, ccfirst;
static u64 monte[MONTEN];
static u64 inmont, mcount;
static double incirc = 2.8147494E14;
static double cexp, montex, montey, montepi, scc, sccun, scclast,
    scct1, scct2, scct3, ent, chisq, datasum;

/*
  Module:       z.c
  Purpose:      compute approximations to normal z distribution probabilities
  Programmer:   Gary Perlman
  Organization: Wang Institute, Tyngsboro, MA 01879
  Copyright:    none
  Tabstops:     4

  poz: probability of normal z value

  PARAM z: normal z value

  Adapted from a polynomial approximation in:
    Ibbetson D, Algorithm 209
    Collected Algorithms of the CACM 1963 p. 616
  Note:
    This routine has six digit accuracy, so it is only useful for absolute
    z values < 6.  For z values >= to 6.0, poz() returns 0.0.
*/
static double ent_poz(const double z)
{
  register double y, x, w;

  if (z == 0.0) {
    x = 0.0;
  } else {
    y = 0.5 * fabs(z);
    if (y >= (Z_MAX * 0.5)) {
      x = 1.0;
    } else if (y < 1.0) {
      w = y * y;
      x = ((((((((0.000124818987 * w - 0.001075204047) * w + 0.005198775019) * w - 0.019198292004) * w + 0.059054035642) * w - 0.151968751364) * w + 0.319152932694) * w - 0.531923007300) * w + 0.797884560593) * y * 2.0;
    } else {
      y -= 2.0;
      x = (((((((((((((-0.000045255659 * y + 0.000152529290) * y - 0.000019538132) * y - 0.000676904986) * y + 0.001390604284) * y - 0.000794620820) * y - 0.002034254874) * y + 0.006549791214) * y - 0.010557625006) * y + 0.011630447319) * y - 0.009279453341) * y + 0.005353579108) * y - 0.002141268741) * y + 0.000535310849) * y + 0.999936657524;
    }
  }

  return (z > 0.0 ? ((x + 1.0) * 0.5) : ((1.0 - x) * 0.5));
}

/*
  Module:       chisq.c
  Purpose:      compute approximations to chisquare distribution probabilities
  Contents:     pochisq()
  Uses:         poz() in z.c (Algorithm 209)
  Programmer:   Gary Perlman
  Organization: Wang Institute, Tyngsboro, MA 01879
  Copyright:    none
  Tabstops:     4

  pochisq: probability of chi sqaure value

  PARAM ax: the obtained chi-square value from previous calculations

  Adapted from:
    Hill, I. D. and Pike, M. C.  Algorithm 299
    Collected Algorithms for the CACM 1967 p. 243
  Updated for rounding errors based on remark in
    ACM TOMS June 1985, page 185
*/
static double ent_pochisq(const double ax)
{
  register double a, e, c, s, x, y, z;

  if (ax <= 0.0)
    return 1.0;

  a = 0.5 * ax;
  y = ex(-a);

  s = 2.0 * ent_poz(-sqrt(ax));
  x = 0.5 * 255;
  z = 0.5;

  if (a > BIGX) {
    e = LOG_SQRT_PI;
    c = log(a);
    while (z <= x) {
      e = log(z) + e;
      s += ex(c * z - a - e);
      z += 1.0;
    }
    return s;
  }

  e = (I_SQRT_PI / sqrt(a));
  c = 0.0;
  while (z <= x) {
    e = e * (a / z);
    c = c + e;
    z += 1.0;
  }
  return (c * y + s);
}

// Need to do a little rewrite to make ent process 8 bytes at a time
// since ADAM's default work unit is 64 bits
void ent_test(u8 *byte)
{
  // Update counters for each bin, and then total
  ++ccount[byte[0]];
  ++ccount[byte[1]];
  ++ccount[byte[2]];
  ++ccount[byte[3]];
  ++ccount[byte[4]];
  ++ccount[byte[5]];
  ++ccount[byte[6]];
  ++ccount[byte[7]];
  totalc += 8;

  register u8 i = 0;
  do {
    /*
      Update inside / outside circle counts
      for Monte Carlo computation of PI
    */

    // Save 6 bytes for Monte Carlo
    monte[mp++] = byte[i];
    if (mp >= MONTEN) {
      // Calculate every MONTEN character
      mp = 0;
      ++mcount;
      montex = montey = 0.0;
      for (u8 mj = 0; mj < (MONTEN >> 1); ++mj) {
        montex = (montex * 256.0) + monte[mj];
        montey = (montey * 256.0) + monte[(MONTEN >> 1) + mj];
      }
      inmont += ((montex * montex + montey * montey) <= incirc);
    }

    sccun = byte[i];
    scct1 += scclast * sccun;
    scct2 += sccun;
    scct3 += (sccun * sccun);
    scclast = sccun;
  } while (++i < 8);
}

void ent_results(ent_report *rsl)
{
  register u16 i;

  // Complete calculation of serial correlation coefficient
  scct1 = scct1 + scclast * rsl->sccu0;
  scct2 *= scct2;
  scc = totalc * scct3 - scct2;
  scc = (scc == 0.0) ? -100000 : ((totalc * scct1 - scct2) / scc);

  /*
    Scan bins and calculate probability for each bin and
    Chi-Square distribution.  The probability will be reused
    in the entropy calculation below.  While we're at it, we
    sum of all the data which will be used to compute the mean.
  */
  register double a;
  cexp = totalc / 256.0; // Expected count per bin
  for (i = 0; i < BUF_SIZE; ++i) {
    a = ccount[i] - cexp;
    prob[i] = ((double)ccount[i]) / totalc;
    chisq += (a * a) / cexp;
    datasum += ((double)i) * ccount[i];
  }

  for (i = 0; i < BUF_SIZE; ++i)
    if (prob[i] > 0.0)
      ent += prob[i] * LOG2(1.0 / prob[i]); // change to (prob[i] > 0.0) * prob[i] * LOG2(1 / prob[i]);

  /*
    Calculate Monte Carlo value for PI from percentage of hits
    within the circle
  */
  montepi = 4.0 * (((double)inmont) / mcount);

  rsl->ent = ent;
  rsl->chisq = chisq;
  rsl->pochisq = ent_pochisq(chisq);
  rsl->mean = datasum / totalc;
  rsl->montepicalc = montepi;
  rsl->monterr = 100.0 * (fabs(PI - montepi) / PI);
  rsl->scc = scc;
  rsl->freq = &ccount[0];
}