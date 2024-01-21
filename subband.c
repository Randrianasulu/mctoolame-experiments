#include "common.h"
#include "decoder.h"
#include "subband.h"

void create_syn_filter (double filter[64][SBLIMIT])
{
  register int i, k;

  for (i = 0; i < 64; i++)
    for (k = 0; k < 32; k++) {
      if ((filter[i][k] =
	   1e9 * cos ((double) ((PI64 * i + PI4) * (2 * k + 1)))) >= 0)
	modf (filter[i][k] + 0.5, &filter[i][k]);
      else
	modf (filter[i][k] - 0.5, &filter[i][k]);
      filter[i][k] *= 1e-9;
    }
}

#define NEWSUB
#ifdef NEWSUB
#include "dewindow.h"
#else
static void read_syn_window (double window[HAN_SIZE])
{
  int i, j[4];
  FILE *fp;
  double f[4];
  char t[150];

  if (!(fp = OpenTableFile ("dewindow"))) {
    fprintf (stderr, "Please check synthesis window table 'dewindow'\n");
    exit (1);
  }
  for (i = 0; i < 512; i += 4) {
    fgets (t, 150, fp);
    sscanf (t, "D[%d] = %lf D[%d] = %lf D[%d] = %lf D[%d] = %lf\n",
	    j, f, j + 1, f + 1, j + 2, f + 2, j + 3, f + 3);
    if (i == j[0]) {
      window[i] = f[0];
      window[i + 1] = f[1];
      window[i + 2] = f[2];
      window[i + 3] = f[3];
    } else {
      fprintf (stderr, "Check index in synthesis window table\n");
      exit (1);
    }
    fgets (t, 80, fp);
  }
  fclose (fp);
}
#endif


int SubBandSynthesis (double *bandPtr, int channel, long *samples)
{
  long foo;
  register int i, j, k;
  register double *bufOffsetPtr, sum;
  static int init = 1;
  typedef double NN[64][32];
  static NN *filter;
  typedef double BB[7][2 * HAN_SIZE];
  static BB *buf;
  static int bufOffset = 64;
  int clip = 0;			/* count & return how many samples clipped */

  if (init) {
    buf = (BB *) mem_alloc (sizeof (BB), "BB");
    filter = (NN *) mem_alloc (sizeof (NN), "NN");
    create_syn_filter (*filter);
    bufOffset = 64;
    init = 0;
  }
  if (channel == 0)
    bufOffset = (bufOffset - 64) & 0x3ff;
  bufOffsetPtr = &((*buf)[channel][bufOffset]);

  for (i = 0; i < 64; i++) {
    sum = 0;
    for (k = 0; k < 32; k++)
      sum += bandPtr[k] * (*filter)[i][k];
    bufOffsetPtr[i] = sum;
  }

  /*  S(i,j) = D(j+32i) * U(j+32i+((i+1)>>1)*64)  */
  /*  samples(i,j) = MWindow(j+32i) * bufPtr(j+32i+((i+1)>>1)*64)  */
  for (j = 0; j < 32; j++) {
    sum = 0;
    for (i = 0; i < 16; i++) {
      k = j + (i << 5);
      sum += window[k] * (*buf)[channel][((k + (((i + 1) >> 1) << 6)) +
					  bufOffset) & 0x3ff];
    }

/*   {long foo = (sum > 0) ? sum * SCALE + 0.5 : sum * SCALE - 0.5; */
/*   {long foo = sum * SCALE;  */

#ifdef	SB_OUTPUT
    sum = bandPtr[j];		/* 960814 FdB for subband sample generation */
#endif

    foo = floor (sum * SCALE + 0.5);
    if (foo >= (long) SCALE) {
      samples[j] = SCALE - 1;
      ++clip;
    } else if (foo < (long) -SCALE) {
      samples[j] = -SCALE;
      ++clip;
    } else
      samples[j] = foo;

  }
  return (clip);
}

/* MFC fixme: i don't think we need a seperate routing for ML */
int SubBandSynthesis_ml (double *bandPtr, int channel, long *samples)
{
  long foo;
  register int i, j, k;
  register double *bufOffsetPtr, sum;
  static int init = 1;
  typedef double NN[64][32];
  static NN *filter;
  typedef double BB[7][2 * HAN_SIZE];
  static BB *buf;
  static int bufOffset = 64;
  int clip = 0;			/* count & return how many samples clipped */

  if (init) {
    buf = (BB *) mem_alloc (sizeof (BB), "BB");
    filter = (NN *) mem_alloc (sizeof (NN), "NN");
    create_syn_filter (*filter);
    bufOffset = 64;
    init = 0;
  }
  if (channel == 0)
    bufOffset = (bufOffset - 64) & 0x3ff;
  bufOffsetPtr = &((*buf)[channel][bufOffset]);

  for (i = 0; i < 64; i++) {
    sum = 0;
    for (k = 0; k < 32; k++)
      sum += bandPtr[k] * (*filter)[i][k];
    bufOffsetPtr[i] = sum;
  }

  /*  S(i,j) = D(j+32i) * U(j+32i+((i+1)>>1)*64)  */
  /*  samples(i,j) = MWindow(j+32i) * bufPtr(j+32i+((i+1)>>1)*64)  */
  for (j = 0; j < 32; j++) {
    sum = 0;
    for (i = 0; i < 16; i++) {
      k = j + (i << 5);
      sum += window[k] * (*buf)[channel][((k + (((i + 1) >> 1) << 6)) +
					  bufOffset) & 0x3ff];
    }

/*   {long foo = (sum > 0) ? sum * SCALE + 0.5 : sum * SCALE - 0.5; */
/*   {long foo = sum * SCALE;  */

#ifdef	SB_OUTPUT
    sum = bandPtr[j];		/* 960814 FdB for subband sample generation */
#endif

    foo = floor (sum * SCALE + 0.5);
    if (foo >= (long) SCALE) {
      samples[j] = SCALE - 1;
      ++clip;
    } else if (foo < (long) -SCALE) {
      samples[j] = -SCALE;
      ++clip;
    } else
      samples[j] = foo;

  }
  return (clip);
}
