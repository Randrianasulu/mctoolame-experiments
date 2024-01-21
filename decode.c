#include "common.h"
#include "decoder.h"
#include "decode.h"

void decode_info (Bit_stream * bs, frame_params * fr_ps)
{
  layer *hdr = fr_ps->header;

  hdr->version = get1bit (bs);
  hdr->lay = 4 - getbits (bs, 2);
  hdr->error_protection = !get1bit (bs);	/* error protect. TRUE/FALSE */
  hdr->bitrate_index = getbits (bs, 4);
  hdr->sampling_frequency = getbits (bs, 2);
  hdr->padding = get1bit (bs);
  hdr->extension = get1bit (bs);
  hdr->mode = getbits (bs, 2);
  hdr->mode_ext = getbits (bs, 2);
  hdr->copyright = get1bit (bs);
  hdr->original = get1bit (bs);
  hdr->emphasis = getbits (bs, 2);
}

/*******************************************************************
*
* The bit allocation information is decoded. 
*
********************************************************************/

void II_decode_bitalloc (Bit_stream * bs,
			 frame_params * fr_ps,
			 unsigned int bit_alloc[7][SBLIMIT], int bits_log)
{
  int i, j;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;
  unsigned int actual_alloc[7][SBLIMIT];

  for (i = 0; i < SBLIMIT; i++)
    for (j = 0; j < stereo; j++)
      actual_alloc[j][i] = 0;

  for (i = 0; i < jsbound; i++)
    for (j = 0; j < stereo; j++)
      actual_alloc[j][i] = bit_alloc[j][i] =
	(char) getbits (bs, (*alloc)[i][0].bits);

  for (i = jsbound; i < sblimit; i++)	/* expand to 2 channels */
    actual_alloc[0][i] = bit_alloc[0][i] = bit_alloc[1][i] =
      (char) getbits (bs, (*alloc)[i][0].bits);

  for (i = sblimit; i < SBLIMIT; i++)
    for (j = 0; j < stereo; j++)
      bit_alloc[j][i] = 0;

  if (bits_log) {
    for (j = 0; j < stereo; j++)
      for (i = 0; i < SBLIMIT; i++) {
	int alloc_bits = 0, alloc_id = bit_alloc[j][i];

	if (actual_alloc[j][i] > 0)
	  alloc_bits =
	    12 * (*alloc)[i][alloc_id].bits * (*alloc)[i][alloc_id].group;

	printf ("alloc_bi[ %d ][ %2d ]= %3d\n", j, i, alloc_bits);
      }
  }
}






/*****************************************************************
*
* The following two functions implement the layer II
* format of scale factor extraction. Layer I involves reading
* 6 bit per subband as scale factor. Layer II requires reading
* first the scfsi which in turn indicate the number of scale factors
* transmitted.
*   Layer II : II_decode_scale
*
*************************** Layer II stuff ***************************/

void II_decode_scale (Bit_stream * bs,
		      frame_params * fr_ps,
		      unsigned int scfsi[7][SBLIMIT],
		      unsigned int bit_alloc[7][SBLIMIT],
		      unsigned int scale_index[7][3][SBLIMIT],
		      int *l, int *m, int scfsi_log)
{
  layer *info = fr_ps->header;
  int stereo = fr_ps->stereo;
  int i, j;
  int px, pci;
  int sblimit = fr_ps->sblimit;

  if (*m == stereo)
    sblimit = fr_ps->sblimit;
  else
    sblimit = fr_ps->sblimit_mc;

  for (i = 0; i < sblimit; i++) {
    for (j = *l; j < *m; j++)	/* 2 bit scfsi */
      if (bit_alloc[j][i])
	scfsi[j][i] = (char) getbits (bs, 2);
      else
	scfsi[j][i] = 4;
  }

  for (i = sblimit; i < SBLIMIT; i++)
    for (j = *l; j < *m; j++)
      scfsi[j][i] = 4;

  if (scfsi_log) {
    for (j = *l; j < *m; j++)
      for (i = 0; i < SBLIMIT; i++) {
	int scf_bits;

	switch (scfsi[j][i]) {
	  /* all three scale factors transmitted */
	case 0:
	  scf_bits = 20;
	  break;
	  /* two scale factors transmitted */
	case 1:
	case 3:
	  scf_bits = 14;
	  break;
	  /* only one scale factor transmitted */
	case 2:
	  scf_bits = 8;
	  break;
	  /* no scale factors transmitted */
	default:
	  scf_bits = 0;
	}
	printf ("scf_bits[ %d ][ %2d ]= %3d\n", j, i, scf_bits);
      }
  }

  /* 3.6.94 R.S. read the prediction coefficients in the mc - part */
  if (*m > stereo && *m < 7)
    if (fr_ps->header->mc_prediction_on == 1)
      for (i = 0; i < 8; i++)
	if (fr_ps->header->mc_prediction[i] == 1) {
	  for (px = 0;
	       px <
	       pred_coef_table[fr_ps->pred_mode][fr_ps->header->
						 dyn_cross_mode[i]]; px++)
	    if (fr_ps->header->mc_predsi[i][px] != 0) {
	      /* predictors are transfered */
	      fr_ps->header->mc_delay_comp[i][px] = getbits (bs, 3);
	      for (pci = 0; pci < fr_ps->header->mc_predsi[i][px]; pci++)
		fr_ps->header->mc_pred_coeff[i][px][pci] = getbits (bs, 8);
	    } else {
	      /* no prediction coef. */
	      fr_ps->header->mc_pred_coeff[i][px][0] = 127;	/* Index 127 -> 0.0 */
	      fr_ps->header->mc_delay_comp[i][px] = 0;
	    }
	}
  /* 31/10/95 Ralf Schwalbe LFE */
  if (*l == stereo)
    if (info->lfe)
      if (info->lfe_alloc)
	info->lfe_scf = getbits (bs, 6);

  for (i = 0; i < sblimit; i++)
    for (j = *l; j < *m; j++)
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
      } else
	scale_index[j][0][i] = scale_index[j][1][i] = scale_index[j][2][i] =
	  SCALE_RANGE - 1;

  for (i = sblimit; i < SBLIMIT; i++)
    for (j = *l; j < *m; j++)
      scale_index[j][0][i] = scale_index[j][1][i] = scale_index[j][2][i] =
	SCALE_RANGE - 1;
}

/**************************************************************
*
* The following two routines take care of reading the
* compressed sample from the bit stream for layer 2.
* For layer 2, if grouping is
* indicated for a particular subband, then the sample size has
* to be read from the bits_group and the merged samples has
* to be decompose into the three distinct samples. Otherwise,
* it is the same for as layer one.
*
**************************************************/


void II_buffer_sample (Bit_stream * bs,
		       frame_params * fr_ps,
		       unsigned int sample[7][3][SBLIMIT],
		       unsigned int bit_alloc[7][SBLIMIT])
{
  int i, j, k, m;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;

  for (i = 0; i < sblimit; i++)
    for (j = 0; j < ((i < jsbound) ? stereo : 1); j++) {
      if (bit_alloc[j][i]) {
	/* check for grouping in subband */
	if ((*alloc)[i][bit_alloc[j][i]].group == 3) {
	  for (m = 0; m < 3; m++) {
	    k = (*alloc)[i][bit_alloc[j][i]].bits;
	    sample[j][m][i] = (unsigned int) getbits (bs, k);
	  }
	} else {		/* bit_alloc = 3, 5, 9 */
	  unsigned int nlevels, c = 0;

	  nlevels = (*alloc)[i][bit_alloc[j][i]].steps;
	  k = (*alloc)[i][bit_alloc[j][i]].bits;
	  c = (unsigned int) getbits (bs, k);

	  for (k = 0; k < 3; k++) {
	    sample[j][k][i] = c % nlevels;
	    c /= nlevels;
	  }
	}
      } else {			/* for no sample transmitted */
	for (k = 0; k < 3; k++)
	  sample[j][k][i] = 0;
      }
      if (stereo == 2 && i >= jsbound)	/* joint stereo : copy L to R */
	for (k = 0; k < 3; k++)
	  sample[1][k][i] = sample[0][k][i];
    }
  for (i = sblimit; i < SBLIMIT; i++)
    for (j = 0; j < stereo; j++)
      for (k = 0; k < 3; k++)
	sample[j][k][i] = 0;
}



/**************************************************************
*
*   Restore the compressed sample to a factional number.
*   first complement the MSB of the sample
*   for Layer II :
*   Use the formula s = s' * c + d
*
**************************************************************/

double c[17] = { 1.33333333333, 1.60000000000, 1.14285714286,
  1.77777777777, 1.06666666666, 1.03225806452,
  1.01587301587, 1.00787401575, 1.00392156863,
  1.00195694716, 1.00097751711, 1.00048851979,
  1.00024420024, 1.00012208522, 1.00006103888,
  1.00003051851, 1.00001525902
};

double d[17] = { 0.500000000, 0.500000000, 0.250000000, 0.500000000,
  0.125000000, 0.062500000, 0.031250000, 0.015625000,
  0.007812500, 0.003906250, 0.001953125, 0.0009765625,
  0.00048828125, 0.00024414063, 0.00012207031,
  0.00006103516, 0.00003051758
};
void II_dequantize_sample (unsigned int sample[7][3][SBLIMIT],
			   unsigned int bit_alloc[7][SBLIMIT],
			   double fraction[7][SBLIMIT][3][12],
			   frame_params * fr_ps, int *z)
{
  int i, j, k, x;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  al_table *alloc = fr_ps->alloc;

  for (i = 0; i < sblimit; i++)
    for (j = 0; j < 3; j++)
      for (k = 0; k < stereo; k++)
	if (bit_alloc[k][i]) {
	  /* locate MSB in the sample */
	  x = 0;
	  while ((1L << x) < (*alloc)[i][bit_alloc[k][i]].steps)
	    x++;

	  /* MSB inversion */
	  if (((sample[k][j][i] >> (x - 1)) & 1) == 1)
	    fraction[k][i][j][*z] = 0.0;
	  else
	    fraction[k][i][j][*z] = -1.0;

	  /* Form a 2's complement sample */
	  fraction[k][i][j][*z] +=
	    (double) (sample[k][j][i] & ((1 << (x - 1)) - 1)) /
	    (double) (1L << (x - 1));

	  /* Dequantize the sample */
	  fraction[k][i][j][*z] += d[(*alloc)[i][bit_alloc[k][i]].quant];
	  fraction[k][i][j][*z] *= c[(*alloc)[i][bit_alloc[k][i]].quant];
	} else
	  fraction[k][i][j][*z] = 0.0;

  for (i = sblimit; i < SBLIMIT; i++)
    for (j = 0; j < 3; j++)
      for (k = 0; k < stereo; k++)
	fraction[k][i][j][*z] = 0.0;
}

void II_lfe_calc (frame_params * fr_ps)
{
  layer *info = fr_ps->header;
  int x, i;
  al_table *alloc = fr_ps->alloc_mc;

  for (i = 0; i < 12; i++) {
    x = 0;
    while ((1L << x) < (*alloc)[0][info->lfe_alloc].steps)
      x++;
    /* MSB inversion */
    if (((info->lfe_spl[i] >> (x - 1)) & 1) == 1)
      info->lfe_spl_fraction[i] = 0.0;
    else
      info->lfe_spl_fraction[i] = -1.0;

    /* Form a 2's complement sample */
    info->lfe_spl_fraction[i] +=
      (double) (info->lfe_spl[i] & ((1 << (x - 1)) - 1)) /
      (double) (1L << (x - 1));

    /* Dequantize the sample */
    info->lfe_spl_fraction[i] += d[(*alloc)[0][info->lfe_alloc].quant];
    info->lfe_spl_fraction[i] *= c[(*alloc)[0][info->lfe_alloc].quant];

    /* Denormalize the sample */
    info->lfe_spl_fraction[i] *= multiple[info->lfe_scf];
  }
}

/************************************************************
*
*   Restore the original value of the sample ie multiply
*    the fraction value by its scalefactor.
*
*************************************************************/

void II_denormalize_sample (double fraction[7][SBLIMIT][3][12],
			    unsigned int scale_index[7][3][SBLIMIT],
			    frame_params * fr_ps, int x, int *z)
{
  int i, j;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;

  for (i = 0; i < sblimit; i++)
    for (j = 0; j < stereo; j++) {
      fraction[j][i][0][*z] *= multiple[scale_index[j][x][i]];
      fraction[j][i][1][*z] *= multiple[scale_index[j][x][i]];
      fraction[j][i][2][*z] *= multiple[scale_index[j][x][i]];
    }
}








