#ifndef __ZIPF_GEN_H__
#define __ZIPF_GEN_H__

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// We borrow this code from https://answers.launchpad.net/polygraph/+faq/1478.
// The corresponding paper is
// http://ldc.usb.ve/~mcuriel/Cursos/WC/spe2003.pdf.gz return a int in [0, n -
// 1];
int popzipf(int n, long double skew) {
  // popZipf(skew) = wss + 1 - floor((wss + 1) ** (x ** skew))
  long double u = rand() / (long double)(RAND_MAX);
  return (int)(n + 1 - floor(pow(n + 1, pow(u, skew)))) - 1;
}

#endif  //__ZIPF_GEN_H__