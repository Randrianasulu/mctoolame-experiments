#include "common.h"
#include "decoder.h"
#include "decode_mc.h"

/* these arrays are in decode.c */
extern double c[17];
extern double d[17];

void mc_header (Bit_stream * bs, frame_params * fr_ps)
{
  layer *hdr = fr_ps->header;
  hdr->ext_bit_stream_present = get1bit (bs);
  if (hdr->ext_bit_stream_present == 1)
    hdr->n_ad_bytes = getbits (bs, 8);
  hdr->center = getbits (bs, 2);
  hdr->surround = getbits (bs, 2);
  hdr->lfe = get1bit (bs);
  hdr->audio_mix = get1bit (bs);	/* large or small room  R.S. */
  hdr->dematrix_procedure = getbits (bs, 2);
  hdr->no_of_multi_lingual_ch = getbits (bs, 3);
  hdr->multi_lingual_fs = get1bit (bs);
  hdr->multi_lingual_layer = get1bit (bs);
  hdr->copyright_ident_bit = get1bit (bs);
  hdr->copyright_ident_start = get1bit (bs);
}


int pred_coef_table[6][16] = { {6, 4, 4, 4, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 0},
{4, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{4, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


void mc_composite_status_info (Bit_stream * bs,
			       frame_params * fr_ps, int tca_log, int dynx_log)
{
  layer *hdr = fr_ps->header;
  int sbgr, pci;


  hdr->tc_sbgr_select = get1bit (bs);
  hdr->dyn_cross_on = get1bit (bs);
  hdr->mc_prediction_on = get1bit (bs);

  if (hdr->tc_sbgr_select == 1) {
    hdr->tc_allocation = getbits (bs, fr_ps->alloc_bits);
    /* tc_allocation is valid for all sbgr R.S. */
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->tc_alloc[sbgr] = hdr->tc_allocation;
  } else {
    hdr->tc_allocation = 0;
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->tc_alloc[sbgr] = getbits (bs, fr_ps->alloc_bits);
  }

  if (tca_log)
    for (sbgr = 0; sbgr < 12; sbgr++)
      printf ("tc_alloc[ %2d ] = %2d\n", sbgr, hdr->tc_alloc[sbgr]);

  if (hdr->dyn_cross_on == 1) {
    hdr->dyn_cross_LR = get1bit (bs);
    for (sbgr = 0; sbgr < 12; sbgr++) {
      hdr->dyn_cross_mode[sbgr] = getbits (bs, fr_ps->dyn_cross_bits);
      /* 960816 FdB dyn_second_stereo added */
      if (hdr->surround == 3)
	hdr->dyn_second_stereo[sbgr] = get1bit (bs);
    }
  } else {
    hdr->dyn_cross_LR = 0;
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->dyn_cross_mode[sbgr] = 0;
  }

  if (dynx_log)
    for (sbgr = 0; sbgr < 12; sbgr++)
      printf ("dynx_mod[ %2d ] = %2d\n", sbgr, hdr->dyn_cross_mode[sbgr]);

  if (hdr->mc_prediction_on == 1) {
    for (sbgr = 0; sbgr < 8; sbgr++) {
      if ((hdr->mc_prediction[sbgr] = get1bit (bs)) == 1) {
/* R.S. read from npredcoef-table max number of coef. for 3/2 configuration */
/* and then the predsi info -> 0    : no prediction    */
/*			    -> 1..3 : 1..3 coefficient */
	for (pci = 0;
	     pci < pred_coef_table[fr_ps->pred_mode][hdr->dyn_cross_mode[sbgr]];
	     pci++)
	  hdr->mc_predsi[sbgr][pci] = getbits (bs, 2);
      }
    }
  }
}

void II_decode_bitalloc_mc (Bit_stream * bs,
			    frame_params * fr_ps,
			    unsigned int bit_alloc[7][SBLIMIT],
			    int *l, int *m, int bits_log)
{
  layer *info = fr_ps->header;
  int i, j, c, sbgr=0;
  int sblimit = fr_ps->sblimit_mc;
  al_table *alloc = fr_ps->alloc_mc;
  unsigned int actual_alloc[7][SBLIMIT];

  for (i = 0; i < SBLIMIT; i++)
    for (j = *l; j < *m; j++)
      actual_alloc[j][i] = 0;

  /* 10/31/95 Ralf Schwalbe LFE */
  if (info->lfe)
    info->lfe_alloc = (char) getbits (bs, (*alloc)[0][0].bits);

  for (i = 0; i < sblimit; i++)
    if (info->dyn_cross_on == 0)
      for (j = *l; j < *m; j++) {
	if ((fr_ps->header->center != 3) || (i < 12) || (j != 2))
	  actual_alloc[j][i] = bit_alloc[j][i] =
	    (char) getbits (bs, (*alloc)[i][0].bits);
	else
	  bit_alloc[j][i] = 0;
    } else {			/* dyn. cross mode */

      if (i == 0)
	sbgr = 0;
      else
	for (c = 1; c < 12; c++) {
	  if ((sb_groups[c - 1] < i) && (i <= sb_groups[c])) {
	    sbgr = c;		/* search the valid subband group */
	    break;
	  }
	}

      /* 960816 FdB new setup for dyn. crosstalk modes */
      if (info->dyn_cross_mode[sbgr] == 0) {
	for (j = *l; j < *m; j++)
	  if (fr_ps->header->center == 3 && i >= 12 && j == 2)
	    bit_alloc[j][i] = 0;
	  else if (info->surround == 3 && info->dyn_second_stereo[sbgr] == 1) {
	    if (info->center != 0 && j == 4)
	      bit_alloc[j][i] = bit_alloc[3][i];
	    else if (info->center == 0 && j == 3)
	      bit_alloc[j][i] = bit_alloc[2][i];
	    else
	      actual_alloc[j][i] = bit_alloc[j][i] =
		(char) getbits (bs, (*alloc)[i][0].bits);
	  } else
	    actual_alloc[j][i] = bit_alloc[j][i] =
	      (char) getbits (bs, (*alloc)[i][0].bits);
      } else if (fr_ps->dyn_cross_bits == 1) {	/* for channel mode 3/0 and 2/1 */
	/* DynX mode has to be 1 */
	if ((info->center == 3) && (i >= 12))	/* 3/0 + phantom center */
	  bit_alloc[2][i] = 0;
	else if (info->tc_alloc[sbgr] == 1)
	  bit_alloc[2][i] = bit_alloc[0][i];
	else if (info->tc_alloc[sbgr] == 2)
	  bit_alloc[2][i] = bit_alloc[1][i];
	else if (info->dyn_cross_LR)
	  bit_alloc[2][i] = bit_alloc[1][i];
	else
	  bit_alloc[2][i] = bit_alloc[0][i];

	if (info->surround == 3) {	/* 3/0 + 2/0 */
	  actual_alloc[3][i] = bit_alloc[3][i] =
	    (char) getbits (bs, (*alloc)[i][0].bits);
	  if (info->dyn_second_stereo[sbgr] == 1)
	    bit_alloc[4][i] = bit_alloc[3][i];
	  else
	    actual_alloc[4][i] = bit_alloc[4][i] =
	      (char) getbits (bs, (*alloc)[i][0].bits);
	}
      } else if (fr_ps->dyn_cross_bits == 3) {	/* for channel mode 3/1 and 2/2 */
	if ((info->center == 3) && (i >= 12))	/* 3/1 + phantom center */
	  bit_alloc[2][i] = 0;
	else if ((info->dyn_cross_mode[sbgr] == 1)
		 || (info->dyn_cross_mode[sbgr] == 4))
	  actual_alloc[2][i] = bit_alloc[2][i] =
	    (char) getbits (bs, (*alloc)[i][0].bits);
	else {
	  /* T2 not transmitted */
	  if ((fr_ps->header->surround == 2 ||	/* 2/2 mode */
	      fr_ps->header->tc_alloc[sbgr] == 1 ||	/* 3/1 mode */
	      fr_ps->header->tc_alloc[sbgr] == 5 ||	/* 3/1 mode (matrix mode 2 only) */
	       fr_ps->header->tc_alloc[sbgr] != 2) //MFC
	      && !fr_ps->header->dyn_cross_LR)
	    bit_alloc[2][i] = bit_alloc[0][i];	/* C, L or Ls from L0 */
	  else
	    bit_alloc[2][i] = bit_alloc[1][i];	/* C, R or Rs from RO */
	}

	if (info->dyn_cross_mode[sbgr] == 2)
	  actual_alloc[3][i] = bit_alloc[3][i] =
	    (char) getbits (bs, (*alloc)[i][0].bits);
	else if (info->dyn_cross_mode[sbgr] == 4)
	  bit_alloc[3][i] = bit_alloc[2][i];
	else {
	  /* T3 not transmitted */
	  if ((fr_ps->header->surround == 2 ||	/* 2/2 mode */
	      fr_ps->header->tc_alloc[sbgr] == 4 ||	/* 3/1 mode */
	      fr_ps->header->tc_alloc[sbgr] == 5 ||	/* 3/1 mode (matrix mode 2 only) */
	      fr_ps->header->tc_alloc[sbgr] < 3) && fr_ps->header->dyn_cross_LR)
	    bit_alloc[3][i] = bit_alloc[1][i];	/* S, R or Rs from R0 */
	  else
	    bit_alloc[3][i] = bit_alloc[0][i];	/* S, L or Ls from LO */
	}
      } else if (fr_ps->dyn_cross_bits == 4) {	/* for channel mode 3/2 */
	/* T2 */
	if ((info->center == 3) && (i >= 12))	/* 3/2 + phantom center */
	  bit_alloc[2][i] = 0;
	else
	  switch (info->dyn_cross_mode[sbgr]) {
	  case 1:		/* T2 contains bitalloc info */
	  case 2:
	  case 4:
	  case 8:
	  case 9:
	  case 10:
	  case 11:
	  case 12:
	  case 14:
	    actual_alloc[2][i] = bit_alloc[2][i] =
	      (char) getbits (bs, (*alloc)[i][0].bits);
	    break;
	  case 3:		/* T2 contains no bitalloc info */
	  case 5:
	  case 6:
	  case 7:
	  case 13:
	    if ((info->tc_alloc[sbgr] == 1) || (info->tc_alloc[sbgr] == 7))
	      bit_alloc[2][i] = bit_alloc[0][i];
	    else if ((info->tc_alloc[sbgr] == 2) || (info->tc_alloc[sbgr] == 6))
	      bit_alloc[2][i] = bit_alloc[1][i];
	    else if (info->dyn_cross_LR)
	      bit_alloc[2][i] = bit_alloc[1][i];
	    else
	      bit_alloc[2][i] = bit_alloc[0][i];
	    break;
	  }

	/* T3 */
	switch (info->dyn_cross_mode[sbgr]) {
	case 1:		/* T3 contains bitalloc info */
	case 3:
	case 5:
	case 8:
	case 10:
	case 13:
	  actual_alloc[3][i] = bit_alloc[3][i] =
	    (char) getbits (bs, (*alloc)[i][0].bits);
	  break;
	case 2:		/* T3 has to copy its bitalloc from T0 */
	case 4:
	case 6:
	case 7:
	case 12:
	  bit_alloc[3][i] = bit_alloc[0][i];
	  break;
	case 9:		/* T3 has to copy its bitalloc from T2 */
	case 11:
	case 14:
	  bit_alloc[3][i] = bit_alloc[2][i];
	  break;
	}

	/* T4 */
	switch (info->dyn_cross_mode[sbgr]) {
	case 2:		/* T4 contains bitalloc info */
	case 3:
	case 6:
	case 9:
	  actual_alloc[4][i] = bit_alloc[4][i] =
	    (char) getbits (bs, (*alloc)[i][0].bits);
	  break;
	case 1:		/* T4 has to copy its bitalloc from T1 */
	case 4:
	case 5:
	case 7:
	case 11:
	  bit_alloc[4][i] = bit_alloc[1][i];
	  break;
	case 10:		/* T4 has to copy its bitalloc from T2 */
	case 12:
	case 14:
	  bit_alloc[4][i] = bit_alloc[2][i];
	  break;
	case 8:		/* T4 has to copy its bitalloc from T3 */
	case 13:
	  bit_alloc[4][i] = bit_alloc[3][i];
	  break;
	}
      }
    }

  for (i = sblimit; i < SBLIMIT; i++)
    for (j = *l; j < *m; j++)
      bit_alloc[j][i] = 0;

  if (bits_log) {
    for (j = *l; j < *m; j++)
      for (i = 0; i < SBLIMIT; i++) {
	int alloc_bits = 0, alloc_id = bit_alloc[j][i];

	if (actual_alloc[j][i] > 0)
	  alloc_bits =
	    12 * (*alloc)[i][alloc_id].bits * (*alloc)[i][alloc_id].group;

	printf ("alloc_bi[ %d ][ %2d ]= %3d\n", j, i, alloc_bits);
      }
  }
}

void II_buffer_sample_mc (Bit_stream * bs,
			  frame_params * fr_ps,
			  unsigned int sample[7][3][SBLIMIT],
			  unsigned int bit_alloc[7][SBLIMIT],
			  int ch_start, int channels, int gr)
{
  layer *info = fr_ps->header;
  int i, j, k, m, sbgr=0, l;
  unsigned int nlevels, c = 0;
  int sblimit = fr_ps->sblimit_mc;
  int transmitted;
  al_table *alloc = fr_ps->alloc_mc;

  /* 31/10/95 Ralf Schwalbe LFE */
  /* 961003 FdB LFE number of bits corrected */
  if (info->lfe && info->lfe_alloc > 0)
    info->lfe_spl[gr] = (unsigned int) getbits (bs, info->lfe_alloc + 1);

  for (i = 0; i < sblimit; i++) {
    if (i == 0)
      sbgr = 0;
    else
      for (l = 1; l < 12; l++)
	if ((sb_groups[l - 1] < i) && (i <= sb_groups[l])) {
	  sbgr = l;
	  break;
	}

    for (j = ch_start; j < channels; j++) {
      if (bit_alloc[j][i]) {
	transmitted = 1;
	if (fr_ps->header->dyn_cross_on == 1) {
	  if (fr_ps->dyn_cross_bits == 4 && ((fr_ps->header->
					      dyn_cross_mode[sbgr] == 1
					      && j == 4)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 2
						 && j == 3)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 3
						 && j == 2)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 4
						 && j != 2)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 5
						 && j != 3)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 6
						 && j != 4)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 7)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 8
						 && j == 4)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 9
						 && j == 3)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 10
						 && j == 4)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 11
						 && j != 2)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 12
						 && j != 2)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 13
						 && j != 3)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 14
						 && j != 2)
	      ))
	    transmitted = 0;	/* 3/2 */
	  if (fr_ps->dyn_cross_bits == 3 && ((fr_ps->header->
					      dyn_cross_mode[sbgr] == 1
					      && j == 3)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 2
						 && j == 2)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 3)
					     || (fr_ps->header->
						 dyn_cross_mode[sbgr] == 4
						 && j == 3)
	      ))
	    transmitted = 0;	/* 3/1 and 2/2 */
	  if (fr_ps->dyn_cross_bits == 1
	      && fr_ps->header->dyn_cross_mode[sbgr] == 1 && j == 2)
	    transmitted = 0;	/* 3/0 (+2/0) and 2/1 */
	  if (fr_ps->header->surround == 3
	      && fr_ps->header->dyn_second_stereo[sbgr] == 1) {
	    if ((fr_ps->header->center == 1 || fr_ps->header->center == 3)
		&& j == 4)
	      transmitted = 0;
	    else if (fr_ps->header->center == 0 && j == 3)
	      transmitted = 0;
	  }
	}
      } else
	transmitted = 0;

      if (transmitted == 1) {
	/* check for grouping in subband */
	if ((*alloc)[i][bit_alloc[j][i]].group == 3) {
	  for (m = 0; m < 3; m++) {
	    k = (*alloc)[i][bit_alloc[j][i]].bits;
	    sample[j][m][i] = (unsigned int) getbits (bs, k);
	  }
	} else {		/* bit_alloc = 3, 5, 9 */
	  nlevels = (*alloc)[i][bit_alloc[j][i]].steps;
	  k = (*alloc)[i][bit_alloc[j][i]].bits;
	  c = (unsigned int) getbits (bs, k);
	  for (k = 0; k < 3; k++) {
	    sample[j][k][i] = c % nlevels;
	    c /= nlevels;
	  }
	}
      } else			/* no samples transmitted */
	for (k = 0; k < 3; k++)
	  sample[j][k][i] = 0;
    }				/* for channel loop */
  }				/* for sblimit loop */

  for (i = sblimit; i < SBLIMIT; i++)
    for (j = ch_start; j < channels; j++)
      for (k = 0; k < 3; k++)
	sample[j][k][i] = 0;
}

void II_dequantize_sample_mc (unsigned int sample[7][3][SBLIMIT],
			      unsigned int bit_alloc[7][SBLIMIT],
			      double fraction[7][SBLIMIT][3][12],
			      frame_params * fr_ps, int ch_start, int channels,
			      int *z)
{
  int i, j, k, x, sbgr=0, l;
  int sblimit = fr_ps->sblimit_mc;
  al_table *alloc = fr_ps->alloc_mc;

  for (i = 0; i < sblimit; i++) {
    if (i == 0)
      sbgr = 0;
    else
      for (l = 1; l < 12; l++)
	if ((sb_groups[l - 1] < i) && (i <= sb_groups[l])) {
	  sbgr = l;
	  break;
	}

    for (j = 0; j < 3; j++)
      for (k = ch_start; k < channels; k++)
	if (bit_alloc[k][i]) {
	  if (fr_ps->header->dyn_cross_on == 0) {
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
	  } else {		/* 21.07.94 Ralf Schwalbe dyn. cross mode */

	    if (fr_ps->dyn_cross_bits == 4 &&
		((fr_ps->header->dyn_cross_mode[sbgr] == 0) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 1 && k != 4) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 2 && k != 3) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 3 && k != 2) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 8 && k != 4) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 9 && k != 3) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 10 && k != 4))) {
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
	    }
	    /* end if 2-channel dyn-cross mode */
	    if (fr_ps->dyn_cross_bits != 4 ||
		((fr_ps->header->dyn_cross_mode[sbgr] == 4 && k == 2) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 5 && k == 3) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 6 && k == 4) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 11 && k == 2) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 12 && k == 2) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 13 && k == 3) ||
		 (fr_ps->header->dyn_cross_mode[sbgr] == 14 && k == 2))) {
	      if ((fr_ps->dyn_cross_bits == 3
		   && ((fr_ps->header->dyn_cross_mode[sbgr] == 1 && k == 3)
		       || (fr_ps->header->dyn_cross_mode[sbgr] == 2 && k == 2)
		       || fr_ps->header->dyn_cross_mode[sbgr] == 3
		       || (fr_ps->header->dyn_cross_mode[sbgr] == 4 && k == 3)
		   )) || (fr_ps->dyn_cross_bits == 1
			  && fr_ps->header->dyn_cross_mode[sbgr] == 1 && k == 2)
		  || (fr_ps->header->surround == 3
		      && fr_ps->header->dyn_second_stereo[sbgr] == 1
		      && ((fr_ps->header->center != 0 && j == 4)
			  || (fr_ps->header->center == 0 && j == 3)))) {
		/* no samples to dequantize */
	      } else {
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
	      }			/* end if ..bits */
	    }			/* end if 1-channel dyn-cross mode */
	  }			/* end if dyn-cross on */
	} /* if bit_alloc */
	else
	  fraction[k][i][j][*z] = 0.0;
  }
  for (i = sblimit; i < SBLIMIT; i++)
    for (j = 0; j < 3; j++)
      for (k = ch_start; k < channels; k++)
	fraction[k][i][j][*z] = 0.0;
}

void II_denormalize_sample_mc (double fraction[7][SBLIMIT][3][12],
			       unsigned int scale_index[7][3][SBLIMIT],
			       frame_params * fr_ps,
			       int x, int ch_start, int channels, int *z)
{
  int i, j, sbgr=0, l, bl = 0;
  int sblimit = fr_ps->sblimit_mc;

  for (i = 0; i < sblimit; i++) {
    if (i == 0)
      sbgr = 0;
    else
      for (l = 1; l < 12; l++)
	if ((sb_groups[l - 1] < i) && (i <= sb_groups[l])) {
	  sbgr = l;
	  break;
	}

    /* 960821 FdB new setup for dyn. crosstalk modes */
    for (bl = 0; bl < 3; bl++)
      if (fr_ps->header->dyn_cross_on == 0)
	for (j = ch_start; j < channels; j++)
	  fraction[j][i][bl][*z] *= multiple[scale_index[j][x][i]];
      else if (fr_ps->dyn_cross_bits == 0) {	/* for channel mode 2/0 (+2/0) */
	if (fr_ps->header->surround == 3) {	/* 2/0 + 2/0 and 1/0 + 2/0 */
	  fraction[ch_start][i][bl][*z] *=
	    multiple[scale_index[ch_start][x][i]];
	  if (fr_ps->header->dyn_second_stereo[sbgr] == 0)
	    fraction[ch_start + 1][i][bl][*z] *=
	      multiple[scale_index[ch_start + 1][x][i]];
	  else
	    fraction[ch_start + 1][i][bl][*z] =
	      (fraction[ch_start][i][bl][*z] /
	       multiple[scale_index[ch_start][x][i]])
	      * multiple[scale_index[ch_start + 1][x][i]];
	}
      } else if (fr_ps->dyn_cross_bits == 1) {	/* for channel mode 3/0 (+2/0) and 2/1 */
	switch (fr_ps->header->dyn_cross_mode[sbgr]) {
	case 0:
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  if (fr_ps->header->surround == 3) {	/* 3/0 + 2/0 */
	    fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	    if (fr_ps->header->dyn_second_stereo[sbgr] == 0)
	      fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	    else
	      fraction[4][i][bl][*z] =
		(fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
		* multiple[scale_index[4][x][i]];
	  }
	  break;
	case 1:
	  if (fr_ps->header->tc_alloc[sbgr] == 0) {
	    if (fr_ps->header->dyn_cross_LR)	/* C,S from R0 */
	      fraction[2][i][bl][*z] =
		(fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
		* multiple[scale_index[2][x][i]];
	    else		/* C,S from L0 */
	      fraction[2][i][bl][*z] =
		(fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
		* multiple[scale_index[2][x][i]];
	  }
	  if (fr_ps->header->tc_alloc[sbgr] == 1)
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  if (fr_ps->header->tc_alloc[sbgr] == 2)
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  if (fr_ps->header->surround == 3) {	/* 3/0 + 2/0 */
	    fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	    if (fr_ps->header->dyn_second_stereo[sbgr] == 0)
	      fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	    else
	      fraction[4][i][bl][*z] =
		(fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
		* multiple[scale_index[4][x][i]];
	  }
	  break;
	}
      } else if (fr_ps->dyn_cross_bits == 3) {	/* for channel mode 3/1 and 2/2 */
	switch (fr_ps->header->dyn_cross_mode[sbgr]) {
	case 0:
	  for (j = ch_start; j < channels; j++)
	    fraction[j][i][bl][*z] *= multiple[scale_index[j][x][i]];
	  break;
	case 1:
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  if (fr_ps->header->surround == 2 ||	/* 2/2 mode */
	      fr_ps->header->tc_alloc[sbgr] == 4 ||	/* 3/1 mode */
	      fr_ps->header->tc_alloc[sbgr] == 5 ||	/* 3/1 mode, matrix mode 2 */
	      (fr_ps->header->tc_alloc[sbgr] != 3
	       && fr_ps->header->dyn_cross_LR))
	    /* S, R or Rs from R0 */
	    fraction[3][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[3][x][i]];
	  else
	    /* S, L or Ls from L0 */
	    fraction[3][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[3][x][i]];
	  break;
	case 2:
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  if (fr_ps->header->surround == 2 ||	/* 2/2 mode */
	      fr_ps->header->tc_alloc[sbgr] == 1 ||	/* 3/1 mode */
	      fr_ps->header->tc_alloc[sbgr] == 5 ||	/* 3/1 mode, matrix mode 2 */
	      (fr_ps->header->tc_alloc[sbgr] != 2
	       && !fr_ps->header->dyn_cross_LR))
	    /* C, L or Ls from L0 */
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else
	    /* C, R or Rs from RO */
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  break;
	case 3:
	  if (fr_ps->header->surround == 2 ||	/* 2/2 mode */
	      fr_ps->header->tc_alloc[sbgr] == 1 ||	/* 3/1 mode */
	      fr_ps->header->tc_alloc[sbgr] == 5 ||	/* 3/1 mode, matrix mode 2 */
	      (fr_ps->header->tc_alloc[sbgr] != 2
	       && !fr_ps->header->dyn_cross_LR))
	    /* C, L or Ls from L0 */
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else
	    /* C, R or Rs from RO */
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  if (fr_ps->header->surround == 2 ||	/* 2/2 mode */
	      fr_ps->header->tc_alloc[sbgr] == 4 ||	/* 3/1 mode */
	      fr_ps->header->tc_alloc[sbgr] == 5 ||	/* 3/1 mode, matrix mode 2 */
	      (fr_ps->header->tc_alloc[sbgr] != 3
	       && fr_ps->header->dyn_cross_LR))
	    /* S, R or Rs from R0 */
	    fraction[3][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[3][x][i]];
	  else
	    /* S, L or Ls from L0 */
	    fraction[3][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[3][x][i]];
	  break;
	case 4:
	  fraction[3][i][bl][*z] = fraction[2][i][bl][*z];
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  break;
	}
      } else if (fr_ps->dyn_cross_bits == 4) {	/* for channel mode 3/2 */
	switch (fr_ps->header->dyn_cross_mode[sbgr]) {
	case 0:
	  for (j = ch_start; j < channels; j++)
	    fraction[j][i][bl][*z] *= multiple[scale_index[j][x][i]];
	  break;
	case 1:		/* C from T2 */
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] =
	    (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	    * multiple[scale_index[4][x][i]];
	  break;
	case 2:		/* C from T2 */
	  fraction[2][i][bl][*z] =
	    fraction[2][i][bl][*z] * multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] =
	    (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	    * multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 3:
	  if ((fr_ps->header->tc_alloc[sbgr] == 1)
	      || (fr_ps->header->tc_alloc[sbgr] == 7))
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if ((fr_ps->header->tc_alloc[sbgr] == 2)
		   || (fr_ps->header->tc_alloc[sbgr] == 6))
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if (fr_ps->header->dyn_cross_LR)	/* C from RO */
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else			/* C from LO */
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];

	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 4:		/* C from T2 */
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] =
	    (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	    * multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] =
	    (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	    * multiple[scale_index[4][x][i]];
	  break;
	case 5:
	  if ((fr_ps->header->tc_alloc[sbgr] == 1)
	      || (fr_ps->header->tc_alloc[sbgr] == 7))
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if ((fr_ps->header->tc_alloc[sbgr] == 2)
		   || (fr_ps->header->tc_alloc[sbgr] == 6))
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if (fr_ps->header->dyn_cross_LR)	/* C from RO */
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else			/* C from LO */
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];

	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] =
	    (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	    * multiple[scale_index[4][x][i]];
	  break;
	case 6:
	  if ((fr_ps->header->tc_alloc[sbgr] == 1)
	      || (fr_ps->header->tc_alloc[sbgr] == 7))
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if ((fr_ps->header->tc_alloc[sbgr] == 2)
		   || (fr_ps->header->tc_alloc[sbgr] == 6))
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if (fr_ps->header->dyn_cross_LR)	/* C from RO */
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else			/* C from LO */
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];

	  fraction[3][i][bl][*z] =
	    (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	    * multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 7:
	  if ((fr_ps->header->tc_alloc[sbgr] == 1)
	      || (fr_ps->header->tc_alloc[sbgr] == 7))
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if ((fr_ps->header->tc_alloc[sbgr] == 2)
		   || (fr_ps->header->tc_alloc[sbgr] == 6))
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if (fr_ps->header->dyn_cross_LR)	/* C from RO */
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else			/* C from LO */
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] =
	    (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	    * multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] =
	    (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	    * multiple[scale_index[4][x][i]];
	  break;
	case 8:		/* C from T2 */
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[4][i][bl][*z] = fraction[3][i][bl][*z];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 9:		/* C from T2 */
	  fraction[3][i][bl][*z] = fraction[2][i][bl][*z];
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 10:		/* C from T2 */
	  fraction[4][i][bl][*z] = fraction[2][i][bl][*z];
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 11:
	  fraction[3][i][bl][*z] = fraction[2][i][bl][*z];
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] =
	    (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	    * multiple[scale_index[4][x][i]];
	  break;
	case 12:
	  fraction[4][i][bl][*z] = fraction[2][i][bl][*z];
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] =
	    (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	    * multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 13:
	  if ((fr_ps->header->tc_alloc[sbgr] == 1)
	      || (fr_ps->header->tc_alloc[sbgr] == 7))
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if ((fr_ps->header->tc_alloc[sbgr] == 2)
		   || (fr_ps->header->tc_alloc[sbgr] == 6))
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else if (fr_ps->header->dyn_cross_LR)	/* C from RO */
	    fraction[2][i][bl][*z] =
	      (fraction[1][i][bl][*z] / multiple[scale_index[1][x][i]])
	      * multiple[scale_index[2][x][i]];
	  else			/* C from LO */
	    fraction[2][i][bl][*z] =
	      (fraction[0][i][bl][*z] / multiple[scale_index[0][x][i]])
	      * multiple[scale_index[2][x][i]];
	  fraction[4][i][bl][*z] = fraction[3][i][bl][*z];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	case 14:
	  fraction[4][i][bl][*z] = fraction[2][i][bl][*z];
	  fraction[3][i][bl][*z] = fraction[2][i][bl][*z];
	  /* C from T2 */
	  fraction[2][i][bl][*z] *= multiple[scale_index[2][x][i]];
	  fraction[3][i][bl][*z] *= multiple[scale_index[3][x][i]];
	  fraction[4][i][bl][*z] *= multiple[scale_index[4][x][i]];
	  break;
	}
      }
  }				/* for sblimit */
}

void dematricing_mc (double pcm_sample[7][SBLIMIT][3][12],
		     frame_params * fr_ps, double pred_buf[2][8][36 + PREDDEL])
{
  int j, jj, k, tc_alloc, l, sbgr = 0;
  //MFC   layer *info = fr_ps->header;
  double tmp_sample, tmp_sample1, surround_sample;

  for (jj = 0; jj < 12; jj++)
    for (j = 0; j < 3; ++j) {
      for (k = 0; k < SBLIMIT; k++) {
	if (fr_ps->header->tc_sbgr_select == 1)
	  tc_alloc = fr_ps->header->tc_allocation;	/* one tc_alloc is valid for all sbgr */
	else {
	  if (k == 0)
	    sbgr = 0;
	  else
	    for (l = 1; l < 12; l++) {
	      if ((sb_groups[l - 1] < k) && (k <= sb_groups[l])) {
		sbgr = l;	/* search the valid subband group */
		break;
	      }
	    }
	  tc_alloc = fr_ps->header->tc_alloc[sbgr];	/* no prediction, but different tc_alloc's
							   per subband */
	}			/* else tc_sbgr_select == 0 */

	if (fr_ps->header->mc_prediction_on && k < 8
	    && fr_ps->header->mc_prediction[k]) {
	  /* prediction on in sbgr */
	  if ((fr_ps->header->surround == 2) && (fr_ps->header->center != 0)) {
	    /* FdB channel mode 3/2 */
	    switch (fr_ps->header->dyn_cross_mode[k]) {	/* if prediction on in sb */
	    case 0:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      pcm_sample[3][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 2)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 3);
	      pcm_sample[4][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 4)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 5);
	      break;

	    case 1:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      pcm_sample[3][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 2)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 3);
	      break;

	    case 2:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      pcm_sample[4][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 2)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 3);
	      break;

	    case 3:
	      pcm_sample[3][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      pcm_sample[4][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 2)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 3);
	      break;

	    case 4:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    case 5:
	      pcm_sample[3][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    case 6:
	      pcm_sample[4][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    case 8:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    case 9:
	      pcm_sample[4][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    case 10:
	      pcm_sample[3][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    }
	  } /* 3/2 mode */
	  else if ((fr_ps->header->surround == 1 && fr_ps->header->center != 0)
		   || (fr_ps->header->surround == 2
		       && fr_ps->header->center == 0)) {
	    /* FdB channel modes 3/1 and 2/2 */
	    switch (fr_ps->header->dyn_cross_mode[k]) {	/* if prediction on in sb */
	    case 0:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      pcm_sample[3][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 2)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 3);
	      break;

	    case 1:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    case 2:
	      pcm_sample[3][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    }
	  } /* 3/1 and 2/2 modes */
	  else
	    if (((fr_ps->header->surround == 0 || fr_ps->header->surround == 3)
		 && fr_ps->header->center != 0) || (fr_ps->header->surround == 1
						    && fr_ps->header->center ==
						    0)) {
	    /* FdB channel modes 3/0 (+ 2/0) and 2/1 */
	    switch (fr_ps->header->dyn_cross_mode[k]) {	/* if prediction on in sb */
	    case 0:
	      pcm_sample[2][k][j][jj] +=
		predict (pred_buf, fr_ps, k, jj, j, 0, 0)
		+ predict (pred_buf, fr_ps, k, jj, j, 1, 1);
	      break;

	    }
	  }
	}

	if (fr_ps->header->dematrix_procedure != 3) { //MFC
	  if ((fr_ps->header->surround == 2) && (fr_ps->header->center != 0)) {
	    /* FdB channel mode 3/2 */
	    switch (tc_alloc) {
	    case 0:
	      if (fr_ps->header->dematrix_procedure == 2) {
		surround_sample =
		  (pcm_sample[3][k][j][jj] + pcm_sample[4][k][j][jj]) / 2.0;
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] +
		  surround_sample;
		pcm_sample[1][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  surround_sample;
	      } else {
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
		pcm_sample[1][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[4][k][j][jj];
	      }
	      break;

	    case 1:
	      if (fr_ps->header->dematrix_procedure == 2) {
		surround_sample =
		  (pcm_sample[3][k][j][jj] + pcm_sample[4][k][j][jj]) / 2.0;
		tmp_sample = pcm_sample[2][k][j][jj];	/* L */
		pcm_sample[2][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] +
		  surround_sample;
		pcm_sample[1][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  surround_sample;
		pcm_sample[0][k][j][jj] = tmp_sample;
	      } else {
		tmp_sample = pcm_sample[2][k][j][jj];	/* L */
		pcm_sample[2][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
		pcm_sample[1][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[4][k][j][jj];
		pcm_sample[0][k][j][jj] = tmp_sample;
	      }
	      break;

	    case 2:
	      if (fr_ps->header->dematrix_procedure == 2) {
		surround_sample =
		  (pcm_sample[3][k][j][jj] + pcm_sample[4][k][j][jj]) / 2.0;
		tmp_sample = pcm_sample[2][k][j][jj];	/* R */
		pcm_sample[2][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  surround_sample;
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] +
		  surround_sample;
		pcm_sample[1][k][j][jj] = tmp_sample;
	      } else {
		tmp_sample = pcm_sample[2][k][j][jj];	/* R */
		pcm_sample[2][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[4][k][j][jj];
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
		pcm_sample[1][k][j][jj] = tmp_sample;
	      }
	      break;

	    case 3:
	      if (fr_ps->header->dematrix_procedure == 2) {
		tmp_sample = pcm_sample[3][k][j][jj];	/* L in T3 */
		pcm_sample[3][k][j][jj] =
		  -2.0 * (pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
			  pcm_sample[3][k][j][jj]) - pcm_sample[4][k][j][jj];
		pcm_sample[1][k][j][jj] =
		  pcm_sample[0][k][j][jj] + pcm_sample[1][k][j][jj] -
		  2.0 * pcm_sample[2][k][j][jj] - tmp_sample;
		pcm_sample[0][k][j][jj] = tmp_sample;
	      } else {
		tmp_sample = pcm_sample[3][k][j][jj];	/* L in T3 */
		pcm_sample[3][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[3][k][j][jj] -
		  pcm_sample[2][k][j][jj];
		pcm_sample[1][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[4][k][j][jj];
		pcm_sample[0][k][j][jj] = tmp_sample;
	      }
	      break;

	    case 4:
	      if (fr_ps->header->dematrix_procedure == 2) {
		tmp_sample = pcm_sample[4][k][j][jj];	/* R in T4 */
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] + pcm_sample[1][k][j][jj] -
		  2.0 * pcm_sample[2][k][j][jj] - pcm_sample[4][k][j][jj];
		pcm_sample[4][k][j][jj] =
		  2.0 * pcm_sample[1][k][j][jj] -
		  2.0 * (pcm_sample[2][k][j][jj] + pcm_sample[4][k][j][jj]) -
		  pcm_sample[3][k][j][jj];
		pcm_sample[1][k][j][jj] = tmp_sample;
	      } else {
		tmp_sample = pcm_sample[4][k][j][jj];	/* R in T4 */
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
		pcm_sample[4][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[4][k][j][jj] -
		  pcm_sample[2][k][j][jj];
		pcm_sample[1][k][j][jj] = tmp_sample;
	      }
	      break;

	    case 5:
	      if (fr_ps->header->dematrix_procedure == 2) {
		tmp_sample = pcm_sample[3][k][j][jj];	/* L in T3 */
		pcm_sample[3][k][j][jj] =
		  0.5 * (pcm_sample[1][k][j][jj] - pcm_sample[0][k][j][jj] +
			 pcm_sample[3][k][j][jj] - pcm_sample[4][k][j][jj]);
		pcm_sample[0][k][j][jj] = tmp_sample;
		pcm_sample[1][k][j][jj] = pcm_sample[4][k][j][jj];	/* R in T4 */
		pcm_sample[4][k][j][jj] = pcm_sample[3][k][j][jj];	/* RS = LS */
	      } else {
		tmp_sample = pcm_sample[3][k][j][jj];
		pcm_sample[3][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[3][k][j][jj] -
		  pcm_sample[2][k][j][jj];
		pcm_sample[0][k][j][jj] = tmp_sample;
		tmp_sample = pcm_sample[4][k][j][jj];
		pcm_sample[4][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[4][k][j][jj] -
		  pcm_sample[2][k][j][jj];
		pcm_sample[1][k][j][jj] = tmp_sample;
	      }
	      break;

	    case 6:
	      if (fr_ps->header->dematrix_procedure == 2) {
		tmp_sample = pcm_sample[2][k][j][jj];	/* R in T2 */
		tmp_sample1 = pcm_sample[3][k][j][jj];	/* L in T3 */
		pcm_sample[3][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[0][k][j][jj] -
		  pcm_sample[2][k][j][jj] + pcm_sample[3][k][j][jj] -
		  pcm_sample[4][k][j][jj];
		pcm_sample[2][k][j][jj] =
		  0.5 * (pcm_sample[0][k][j][jj] + pcm_sample[1][k][j][jj] -
			 pcm_sample[2][k][j][jj] - tmp_sample1);
		pcm_sample[1][k][j][jj] = tmp_sample;
		pcm_sample[0][k][j][jj] = tmp_sample1;
	      } else {
		tmp_sample = pcm_sample[2][k][j][jj];	/* R in T2 */
		pcm_sample[2][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[4][k][j][jj];
		pcm_sample[1][k][j][jj] = tmp_sample;
		tmp_sample = pcm_sample[3][k][j][jj];	/* L in T3 */
		pcm_sample[3][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[3][k][j][jj] -
		  pcm_sample[2][k][j][jj];
		pcm_sample[0][k][j][jj] = tmp_sample;
	      }
	      break;

	    case 7:
	      if (fr_ps->header->dematrix_procedure == 2) {
		tmp_sample = pcm_sample[2][k][j][jj];	/* L in T2 */
		tmp_sample1 = pcm_sample[4][k][j][jj];	/* R in T4 */
		pcm_sample[4][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[0][k][j][jj] +
		  pcm_sample[2][k][j][jj] - pcm_sample[3][k][j][jj] -
		  pcm_sample[4][k][j][jj];
		pcm_sample[2][k][j][jj] =
		  0.5 * (pcm_sample[0][k][j][jj] + pcm_sample[1][k][j][jj] -
			 pcm_sample[2][k][j][jj] - tmp_sample1);
		pcm_sample[0][k][j][jj] = tmp_sample;
		pcm_sample[1][k][j][jj] = tmp_sample1;
	      } else {
		tmp_sample = pcm_sample[2][k][j][jj];
		pcm_sample[2][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
		pcm_sample[0][k][j][jj] = tmp_sample;
		tmp_sample = pcm_sample[4][k][j][jj];
		pcm_sample[4][k][j][jj] =
		  pcm_sample[1][k][j][jj] - pcm_sample[4][k][j][jj] -
		  pcm_sample[2][k][j][jj];
		pcm_sample[1][k][j][jj] = tmp_sample;
	      }
	      break;

	    }			/* switch end loop */
	  } else if (fr_ps->header->surround == 1 && fr_ps->header->center != 0) {
	    /* FdB channel mode 3/1 */
	    switch (tc_alloc) {
	    case 0:
	      if (fr_ps->header->dematrix_procedure == 2)
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] +
		  pcm_sample[3][k][j][jj];
	      else
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
	      pcm_sample[1][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		pcm_sample[3][k][j][jj];
	      break;

	    case 1:
	      tmp_sample = pcm_sample[2][k][j][jj];	/* L */
	      if (fr_ps->header->dematrix_procedure == 2)
		pcm_sample[2][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] +
		  pcm_sample[3][k][j][jj];
	      else
		pcm_sample[2][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
	      pcm_sample[1][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		pcm_sample[3][k][j][jj];
	      pcm_sample[0][k][j][jj] = tmp_sample;
	      break;

	    case 2:
	      tmp_sample = pcm_sample[2][k][j][jj];	/* R */
	      pcm_sample[2][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		pcm_sample[3][k][j][jj];
	      if (fr_ps->header->dematrix_procedure == 2)
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] +
		  pcm_sample[3][k][j][jj];
	      else
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
	      pcm_sample[1][k][j][jj] = tmp_sample;
	      break;

	    case 3:
	      tmp_sample = pcm_sample[3][k][j][jj];	/* L in T3 */
	      if (fr_ps->header->dematrix_procedure == 2)
		pcm_sample[3][k][j][jj] =
		  -pcm_sample[0][k][j][jj] + pcm_sample[2][k][j][jj] +
		  pcm_sample[3][k][j][jj];
	      else
		pcm_sample[3][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
	      pcm_sample[1][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		pcm_sample[3][k][j][jj];
	      pcm_sample[0][k][j][jj] = tmp_sample;
	      break;

	    case 4:
	      tmp_sample = pcm_sample[3][k][j][jj];	/* R in T3 */
	      pcm_sample[3][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj] -
		pcm_sample[3][k][j][jj];
	      if (fr_ps->header->dematrix_procedure == 2)
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] +
		  pcm_sample[3][k][j][jj];
	      else
		pcm_sample[0][k][j][jj] =
		  pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj] -
		  pcm_sample[3][k][j][jj];
	      pcm_sample[1][k][j][jj] = tmp_sample;
	      break;

	    case 5:
	      tmp_sample = pcm_sample[2][k][j][jj];	/* L in T2 */
	      tmp_sample1 = pcm_sample[3][k][j][jj];	/* R in T3 */
	      pcm_sample[2][k][j][jj] =
		0.5 * (pcm_sample[0][k][j][jj] + pcm_sample[1][k][j][jj] -
		       tmp_sample - tmp_sample1);
	      pcm_sample[3][k][j][jj] =
		0.5 * (pcm_sample[1][k][j][jj] - pcm_sample[0][k][j][jj] +
		       tmp_sample - tmp_sample1);
	      pcm_sample[0][k][j][jj] = tmp_sample;
	      pcm_sample[1][k][j][jj] = tmp_sample1;
	      break;
	    }			/* switch end */
	  } else if (fr_ps->header->surround == 1 || fr_ps->header->center != 0) {
	    /* FdB channel modes 3/0 (+ 2/0) and 2/1 */
	    switch (tc_alloc) {
	    case 0:
	      pcm_sample[0][k][j][jj] =
		pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[1][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj];
	      break;

	    case 1:
	      tmp_sample = pcm_sample[2][k][j][jj];	/* L */
	      pcm_sample[2][k][j][jj] =
		pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[1][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[0][k][j][jj] = tmp_sample;
	      break;

	    case 2:
	      tmp_sample = pcm_sample[2][k][j][jj];	/* R */
	      pcm_sample[2][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[0][k][j][jj] =
		pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[1][k][j][jj] = tmp_sample;
	      break;
	    }			/* switch end */
	  } else if (fr_ps->header->surround == 2) {
	    /* FdB channel mode 2/2 */
	    switch (tc_alloc) {
	    case 0:
	      pcm_sample[0][k][j][jj] =
		pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[1][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[3][k][j][jj];
	      break;

	    case 1:
	      tmp_sample = pcm_sample[2][k][j][jj];	/* L */
	      pcm_sample[1][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[3][k][j][jj];
	      pcm_sample[2][k][j][jj] =
		pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[0][k][j][jj] = tmp_sample;
	      break;

	    case 2:
	      tmp_sample = pcm_sample[3][k][j][jj];	/* R */
	      pcm_sample[0][k][j][jj] =
		pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[3][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[3][k][j][jj];
	      pcm_sample[1][k][j][jj] = tmp_sample;
	      break;

	    case 3:
	      tmp_sample = pcm_sample[2][k][j][jj];	/* L */
	      pcm_sample[2][k][j][jj] =
		pcm_sample[0][k][j][jj] - pcm_sample[2][k][j][jj];
	      pcm_sample[0][k][j][jj] = tmp_sample;
	      tmp_sample = pcm_sample[3][k][j][jj];	/* R */
	      pcm_sample[3][k][j][jj] =
		pcm_sample[1][k][j][jj] - pcm_sample[3][k][j][jj];
	      pcm_sample[1][k][j][jj] = tmp_sample;
	      break;
	    }			/* switch end */
	  }
	}//MFC
      }			/* for k < sblimit loop */
    }				/* for j < 3 loop */
}

void denormalizing_mc (double pcm_sample[7][SBLIMIT][3][12],
		       frame_params * fr_ps, int channels)
{
  double matr1=0.0;			/* normalizing factor */
  double matr2=0.0;			/* matricing factor   */
  double matr3=0.0;			/* matricing factor   */
  int i, j, jj, k, l;
  layer *info = fr_ps->header;
  int stereo = fr_ps->stereo;

  switch (info->dematrix_procedure) {
    /* factors according to International Standard */
  case 0:
  case 2:
    matr1 = 1 + sqrt (2.0);	/* factor for L and R   */
    matr2 = sqrt (2.0) * matr1;	/* factor for C */
    matr3 = sqrt (2.0) * matr1;	/* factor for Ls, Rs */
    break;
  case 1:
    matr1 = (1.5 + 0.5 * sqrt (2.0));	/* factor for L, R      */
    matr2 = sqrt (2.0) * matr1;	/* factor for C         */
    matr3 = 2 * matr1;		/* factor for Ls, Rs  */
    break;
  case 3:
    matr1 = 1.0;
    matr2 = 1.0;
    matr3 = 1.0;
    break;
  }

  /* denormalized signals */
  if (fr_ps->header->dematrix_procedure != 3)	/* dematrixing */
    for (jj = 0; jj < 12; jj++)
      for (j = 0; j < 3; ++j)
	for (k = 0; k < SBLIMIT; k++) {	/* Lo / Ro */
	  for (i = 0; i < stereo; i++)
	    pcm_sample[i][k][j][jj] = pcm_sample[i][k][j][jj] * matr1;

	  if (fr_ps->header->dematrix_procedure != 1) {	/* matrix 0 and 2, since C, Ls, Rs, and S all use the same value */
	    /* second stereo channels */
	    if (fr_ps->header->surround == 3) {
	      if (fr_ps->header->center != 0)
		pcm_sample[2][k][j][jj] = pcm_sample[2][k][j][jj] * matr2;
	    } else {
	      for (l = 2; l < channels; l++)
		pcm_sample[l][k][j][jj] = pcm_sample[l][k][j][jj] * matr2;
	    }
	  } else {		/* matrix 1 */

	    if (fr_ps->header->surround == 3) {
	      if (fr_ps->header->center != 0)
		pcm_sample[2][k][j][jj] = pcm_sample[2][k][j][jj] * matr2;
	    } else if (fr_ps->mc_channel == 3) {	/* R.S. matr2 = C */
	      pcm_sample[2][k][j][jj] = pcm_sample[2][k][j][jj] * matr2;
	      pcm_sample[3][k][j][jj] = pcm_sample[3][k][j][jj] * matr3;
	      pcm_sample[4][k][j][jj] = pcm_sample[4][k][j][jj] * matr3;
	    } else if (fr_ps->mc_channel == 2) {
	      if (fr_ps->header->surround == 2) {	/* 2/2 */
		pcm_sample[2][k][j][jj] = pcm_sample[2][k][j][jj] * matr3;
		pcm_sample[3][k][j][jj] = pcm_sample[3][k][j][jj] * matr3;
	      } else {		/* 3/1 */

		pcm_sample[2][k][j][jj] = pcm_sample[2][k][j][jj] * matr2;
		pcm_sample[3][k][j][jj] = pcm_sample[3][k][j][jj] * matr3;
	      }
	    } else {		/* mc_channel == 1 */

	      if (fr_ps->header->center == 0)	/* 2/1 */
		pcm_sample[2][k][j][jj] = pcm_sample[2][k][j][jj] * matr3;
	      else		/* 3/0 */
		pcm_sample[2][k][j][jj] = pcm_sample[2][k][j][jj] * matr2;
	    }
	  }
	}
}
/*******************************************************/
/* prediction corrected, Heiko Purnhagen 08-nov-94 */
/*******************************************************/

/* this is only a local function */
double predict (double pred_buf[2][8][36 + PREDDEL],
		       frame_params * fr_ps,
		       int sb, int jj, int j, int ch, int idx)
{
  int i;
  double t;

  t = 0;
  for (i = 0; i < fr_ps->header->mc_predsi[sb][idx]; i++)
    t +=
      pred_buf[ch][sb][PREDDEL + 3 * jj + j - i -
		       fr_ps->header->mc_delay_comp[sb][idx]]
      * (fr_ps->header->mc_pred_coeff[sb][idx][i] - 127) * STEP;
  return t;
}
