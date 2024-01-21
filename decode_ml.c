#include "common.h"
#include "decoder.h"
#include "decode_ml.h"

/* these arrays are in decode.c */
extern double c[17];
extern double d[17];

void II_decode_bitalloc_ml (Bit_stream * bs,
			    frame_params * fr_ps,
			    unsigned int bit_alloc_ml[7][SBLIMIT], int *m)
{
  int i, j;
  int sblimit_ml = fr_ps->sblimit_ml;
  al_table *alloc_ml = fr_ps->alloc_ml;

  /* JR: no dynamic crosstalk for multilingual channels */
  /* JR: no phantom center coding for multilingual channels */
  /* JR: no joint coding for multilingual channels */
  /* JR: they're really simple, aren't they? */

  for (i = 0; i < sblimit_ml; i++)
    for (j = 0; j < *m; j++)
      bit_alloc_ml[j][i] = (char) getbits (bs, (*alloc_ml)[i][0].bits);
  for (i = sblimit_ml; i < SBLIMIT; i++)
    for (j = 0; j < *m; j++)
      bit_alloc_ml[j][i] = 0;
}

void II_decode_scale_ml (Bit_stream * bs,
			 frame_params * fr_ps,
			 unsigned int scfsi[7][SBLIMIT],
			 unsigned int bit_alloc[7][SBLIMIT],
			 unsigned int scale_index[7][3][SBLIMIT], int *m)
{
  int i, j;
  //MFC  int px, pci;
  int sblimit_ml = fr_ps->sblimit_ml;


  for (i = 0; i < sblimit_ml; i++)
    for (j = 0; j < *m; j++)	/* 2 bit scfsi */
      if (bit_alloc[j][i]) {
	scfsi[j][i] = (char) getbits (bs, 2);
      } else
	scfsi[j][i] = 4;
  for (i = sblimit_ml; i < SBLIMIT; i++)
    for (j = 0; j < *m; j++)
      scfsi[j][i] = 4;

  for (i = 0; i < sblimit_ml; i++)
    for (j = 0; j < *m; j++) {
      if (bit_alloc[j][i])
	switch (scfsi[j][i]) {
	  /* all three scale factors transmitted */
	case 0:
	  scale_index[j][0][i] = getbits (bs, 6);
	  scale_index[j][1][i] = getbits (bs, 6);
	  scale_index[j][2][i] = getbits (bs, 6);
	  break;
	  /* scale factor 1 & 3 transmitted */
	case 1:
	  scale_index[j][0][i] = scale_index[j][1][i] = getbits (bs, 6);
	  scale_index[j][2][i] = getbits (bs, 6);
	  break;
	  /* scale factor 1 & 2 transmitted */
	case 3:
	  scale_index[j][0][i] = getbits (bs, 6);
	  scale_index[j][1][i] = scale_index[j][2][i] = getbits (bs, 6);
	  break;
	  /* only one scale factor transmitted */
	case 2:
	  scale_index[j][0][i] =
	    scale_index[j][1][i] = scale_index[j][2][i] = getbits (bs, 6);
	  break;
	default:
	  scale_index[j][0][i] =
	    scale_index[j][1][i] = scale_index[j][2][i] = SCALE_RANGE - 1;
	  break;
      } else {
	scale_index[j][0][i] = scale_index[j][1][i] =
	  scale_index[j][2][i] = SCALE_RANGE - 1;
      }
    }

  for (i = sblimit_ml; i < SBLIMIT; i++)
    for (j = 0; j < *m; j++) {
      scale_index[j][0][i] = scale_index[j][1][i] =
	scale_index[j][2][i] = SCALE_RANGE - 1;
    }
}


void II_buffer_sample_ml (Bit_stream * bs,
			  frame_params * fr_ps,
			  unsigned int sample_ml[7][3][SBLIMIT],
			  unsigned int bit_alloc_ml[7][SBLIMIT], int *n_ml_ch)
{
  int i, j, k, m;
  unsigned int nlevels, c = 0;
  int sblimit_ml = fr_ps->sblimit_ml;
  al_table *alloc_ml = fr_ps->alloc_ml;

  for (i = 0; i < sblimit_ml; i++) {
    for (j = 0; j < *n_ml_ch; j++) {
      if (bit_alloc_ml[j][i]) {
	/* JR: no dynamic crosstalk for multilingual channels */
	/* check for grouping in subband */
	if ((*alloc_ml)[i][bit_alloc_ml[j][i]].group == 3) {

	  for (m = 0; m < 3; m++) {
	    k = (*alloc_ml)[i][bit_alloc_ml[j][i]].bits;
	    sample_ml[j][m][i] = (unsigned int) getbits (bs, k);
	  }

	} else {		/* bit_alloc = 3, 5, 9 */
	  nlevels = (*alloc_ml)[i][bit_alloc_ml[j][i]].steps;
	  k = (*alloc_ml)[i][bit_alloc_ml[j][i]].bits;
	  c = (unsigned int) getbits (bs, k);
	  for (k = 0; k < 3; k++) {
	    sample_ml[j][k][i] = c % nlevels;
	    c /= nlevels;
	  }
	}
      } else {			/* for no sample transmitted */
	for (k = 0; k < 3; k++)
	  sample_ml[j][k][i] = 0;
      }
    }
  }

  for (i = sblimit_ml; i < SBLIMIT; i++)
    for (j = 0; j < *n_ml_ch; j++)
      for (k = 0; k < 3; k++)
	sample_ml[j][k][i] = 0;
}


void II_dequantize_sample_ml (unsigned int sample_ml[7][3][SBLIMIT],
			      unsigned int bit_alloc_ml[7][SBLIMIT],
			      double fraction_ml[7][SBLIMIT][3][12],
			      frame_params * fr_ps, int *n_ml_ch, int *z)
{
  int i, j, k, x;
  int sblimit_ml = fr_ps->sblimit_ml;
  al_table *alloc_ml = fr_ps->alloc_ml;

  for (i = 0; i < sblimit_ml; i++) {
    for (j = 0; j < 3; j++)
      for (k = 0; k < *n_ml_ch; k++)
	if (bit_alloc_ml[k][i]) {
	  /* JR: ditto */
	  /* locate MSB in the sample */
	  x = 0;
	  while ((1L << x) < (*alloc_ml)[i][bit_alloc_ml[k][i]].steps)
	    x++;
	  /* MSB inversion */
	  if (((sample_ml[k][j][i] >> (x - 1)) & 1) == 1)
	    fraction_ml[k][i][j][*z] = 0.0;
	  else
	    fraction_ml[k][i][j][*z] = -1.0;

	  /* Form a 2's complement sample */
	  fraction_ml[k][i][j][*z] +=
	    (double) (sample_ml[k][j][i] & ((1 << (x - 1)) - 1)) /
	    (double) (1L << (x - 1));

	  /* Dequantize the sample */
	  fraction_ml[k][i][j][*z] +=
	    d[(*alloc_ml)[i][bit_alloc_ml[k][i]].quant];
	  fraction_ml[k][i][j][*z] *=
	    c[(*alloc_ml)[i][bit_alloc_ml[k][i]].quant];
	} /* if bit_alloc */
	else
	  fraction_ml[k][i][j][*z] = 0.0;
  }
  for (i = sblimit_ml; i < SBLIMIT; i++)
    for (j = 0; j < 3; j++)
      for (k = 0; k < *n_ml_ch; k++)
	fraction_ml[k][i][j][*z] = 0.0;
}


void II_denormalize_sample_ml (double fraction_ml[7][SBLIMIT][3][12],
			       unsigned int scale_index_ml[7][3][SBLIMIT],
			       frame_params * fr_ps,
			       int x, int *n_ml_ch, int *z)
{
  int i, j;
  int sblimit_ml = fr_ps->sblimit_ml;

  for (i = 0; i < sblimit_ml; i++)
    for (j = 0; j < *n_ml_ch; j++) {
      fraction_ml[j][i][0][*z] *= multiple[scale_index_ml[j][x][i]];
      fraction_ml[j][i][1][*z] *= multiple[scale_index_ml[j][x][i]];
      fraction_ml[j][i][2][*z] *= multiple[scale_index_ml[j][x][i]];
    }
}
