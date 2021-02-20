#include "R.h"
#include "stdio.h"
#include "stdlib.h"

/* ------------------------------------------------------------------------ */
void GenerateMls(int *mls, int *N)
{
  const int maxNoTaps = 18;
  const int tapsTab[16][18] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
      0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
      0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int taps[maxNoTaps];
  int i, j, sum;

  int *delayLine = (int *) Calloc( maxNoTaps, int );

  int P = (1 << *N) - 1;

  for (i = 0; i < *N; i++) // copy the N’th taps table
  {
    taps[i] = tapsTab[maxNoTaps - *N][i];
    delayLine[i] = 1;
  }

  for (i = 0; i < P; i++) // Generate an MLS by summing the taps mod 2
  {
    sum = 0;
    for (j = 0; j < *N; j++)
    {
      sum += taps[j] * delayLine[j];
    }

    sum &= 1; // mod 2

    mls[i] = delayLine[*N - 1];

    for (j = *N - 2; j >= 0; j--)
    {
      delayLine[j + 1] = delayLine[j];
    }
  
    delayLine[0] = *(int*)&sum;
  }

  Free(delayLine);
}

/* ------------------------------------------------------------------------ */
void GenerateSignal(int *mls, double *signal, int *P)
{
  int i;

  double *input = (double *) Calloc(*P, double);

  for (i = 0; i < *P; i++) {  // Change 0 to 1 and 1 to -1
    input[i] = -2 * mls[i] + 1;
  }

  for (i = 0; i < *P; i++) {  // Simulate a system with h = {2, 0.4, 0.2, -0.1, -0.8}, just an example
    signal[i] =
      2.0 * input[(*P + i - 0) % *P]
    + 0.4 * input[(*P + i - 1) % *P]
    + 0.2 * input[(*P + i - 2) % *P]
    - 0.1 * input[(*P + i - 3) % *P]
    - 0.8 * input[(*P + i - 4) % *P];
  }

  Free(input);
}

/* ------------------------------------------------------------------------ */
void FastHadamard(double *x, int P1, int N)
{
  int i, i1, j, k, k1, k2;
  double temp;

  k1 = P1;
  for (k = 0; k < N; k++)
  {
    k2 = k1 >> 1;
    for (j = 0; j < k2; j++)
    {
      for (i = j; i < P1; i = i + k1)
      {
        i1 = i + k2;
        temp = x[i] + x[i1];
        x[i1] = x[i] - x[i1];
        x[i] = temp;
      }
    }
    k1 = k1 >> 1;
  }
}

/* ------------------------------------------------------------------------ */
void PermuteSignal(double *sig, double *perm, int *tagS, int P)
{
  int i;
  double dc = 0;

  for (i = 0; i < P; i++)
  dc += sig[i];
  perm[0] = -dc;
  for (i = 0; i < P; i++) // Just a permutation of the measured signal
   perm[tagS[i]] = sig[i];
}

/* ------------------------------------------------------------------------ */
void PermuteResponse(double *perm, double *resp, int *tagL, int P)
{
  int i;
  const double fact = 1.0 / (double)(P + 1);
  for (i = 0; i < P; i++) // Just a permutation of the impulse response
  {
    resp[i] = perm[tagL[i]] * fact;
  }
  resp[P] = 0;
}

/* ------------------------------------------------------------------------ */
void GeneratetagL(int *mls, int *tagL, int P, int N)
{
  int i, j;

  int *colSum = (int *) Calloc( P, int );
  int *index = (int *) Calloc( N, int );


  for (i = 0; i < P; i++) // Run through all the columns in the autocorr matrix
  {
    colSum[i] = 0;
    // Find colSum as the value of the first N elements regarded as a binary number
    for (j = 0; j < N; j++)
    {
      colSum[i] += mls[(P + i - j) % P] << (N - 1 - j);
    }
    // Figure out if colSum is a 2^j number and store the column as the j’th index
    for ( j = 0; j < N; j++)
    {
      if (colSum[i] == (1 << j))
      index[j] = i;
    }
  }

  for (i = 0; i < P; i++) // For each row in the L matrix
  {
    tagL[i] = 0;
    // Find the tagL as the value of the rows in the L matrix regarded as a binary number
    for ( j = 0; j < N; j++)
    {
      tagL[i] += mls[(P + index[j] - i) % P] * (1 << j);
    }
  }

  Free(colSum);
  Free(index);
}

/* ------------------------------------------------------------------------ */
void GeneratetagS(int *mls, int *tagS, int P, int N)
{
  int i, j;

  for (i = 0; i < P; i++) // For each column in the S matrix
  {
    tagS[i] = 0;
    // Find the tagS as the value of the columns in the S matrix regarded as a binary number
    for (j = 0; j < N; j++)
    {
      tagS[i] += mls[(P + i - j) % P] * (1 << (N - 1 - j));
    }
  }
}

/* ------------------------------------------------------------------------ */
void RecoverImpulseResp(int *mls, double *signal, double *resp, int *N, int *P)
{
  int i;

  int *tagL = (int *) Calloc( *P, int );
  int *tagS = (int *) Calloc( *P, int );
  double *perm = (double *) Calloc( *P + 1, double );

  GeneratetagL( mls, tagL, *P, *N );       // Generate tagL for the L matrix
  GeneratetagS( mls, tagS, *P, *N );       // Generate tagS for the S matrix
  PermuteSignal( signal, perm, tagS, *P ); // Permute the signal according to tagS
  FastHadamard( perm, *P + 1, *N );        // Do a Hadamard transform in place
  PermuteResponse( perm, resp, tagL, *P ); // Permute the impulse response according to tagL

  Free(tagL);
  Free(tagS);
  Free(perm);
}

