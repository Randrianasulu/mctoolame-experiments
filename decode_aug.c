#ifdef Augmentation_7ch
#include "common.h"
#include "decode_aug.h"

void II_denormalize_sample_aug (double fraction[7][SBLIMIT][3][12],
				unsigned int scale_index[7][3][SBLIMIT],
				frame_params * fr_ps, int x, int *z)
{
  layer *info = fr_ps->header;
  int i, j, k, sbgr, l, bl = 0;
  int sblimit = fr_ps->sblimit_mc;

  for (i = 0; i < sblimit; i++) {
    int T5T0 = 0, T6T0 = 0, T6T1 = 1;

    if (i == 0)
      sbgr = 0;
    else
      for (l = 1; l < 12; l++)
	if ((sb_groups[l - 1] < i) && (i <= sb_groups[l])) {
	  sbgr = l;
	  break;
	}

    if (info->tc_aug_alloc[sbgr] == 4 || info->tc_aug_alloc[sbgr] == 5)
      if (info->dyn_cross_LR == 0)
	T6T1 = 0;
      else
	T6T0 = 1;
    else if (info->tc_aug_alloc[sbgr] == 6 || info->tc_aug_alloc[sbgr] == 7)
      if (info->dyn_cross_LR)
	T5T0 = 1;


    /* 960821 FdB new setup for dyn. crosstalk modes */
    for (bl = 0; bl < 3; bl++) {
      switch (fr_ps->header->dyn_cross_aug_mode[sbgr]) {
      case 0:			/* T5 and T6 present */
	for (j = 5; j < 7; j++)
	  fraction[j][i][bl][*z] *= multiple[scale_index[j][x][i]];
	break;
      case 1:			/* T6 from T1 */
	fraction[5][i][bl][*z] *= multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[T6T1][i][bl][*z] / multiple[scale_index[T6T1][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 2:			/* T6 from T2 */
	fraction[5][i][bl][*z] *= multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 3:			/* T6 from T4 */
	fraction[5][i][bl][*z] *= multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[4][i][bl][*z] / multiple[scale_index[4][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 4:			/* T6 from T5 */
	fraction[5][i][bl][*z] *= multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[5][i][bl][*z] / multiple[scale_index[5][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 5:			/* T5 from T0 */
	fraction[5][i][bl][*z] =
	  (fraction[T5T0][i][bl][*z] / multiple[scale_index[T5T0][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] *= multiple[scale_index[6][x][i]];
	break;
      case 6:			/* T5 from T0 and T6 from T1 */
	fraction[5][i][bl][*z] =
	  (fraction[T5T0][i][bl][*z] / multiple[scale_index[T5T0][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[T6T1][i][bl][*z] / multiple[scale_index[T6T1][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 7:			/* T5 from T0 and T6 from T2 */
	fraction[5][i][bl][*z] =
	  (fraction[T5T0][i][bl][*z] / multiple[scale_index[T5T0][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 8:			/* T5 from T0 and T6 from T4 */
	fraction[5][i][bl][*z] =
	  (fraction[T5T0][i][bl][*z] / multiple[scale_index[T5T0][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[4][i][bl][*z] / multiple[scale_index[4][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 9:			/* T5 and T6 from T0 */
	fraction[5][i][bl][*z] =
	  (fraction[T5T0][i][bl][*z] / multiple[scale_index[T5T0][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[T6T0][i][bl][*z] / multiple[scale_index[T6T0][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 10:			/* T5 from T2 */
	fraction[5][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] *= multiple[scale_index[6][x][i]];
	break;
      case 11:			/* T5 from T2 and T6 from T1 */
	fraction[5][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[T6T1][i][bl][*z] / multiple[scale_index[T6T1][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 12:			/* T5 and T6 from T2 */
	fraction[5][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 13:			/* T5 from T2 and T6 from T4 */
	fraction[5][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[4][i][bl][*z] / multiple[scale_index[4][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 14:			/* T5 from T3 */
	fraction[5][i][bl][*z] =
	  (fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] *= multiple[scale_index[6][x][i]];
	break;
      case 15:			/* T5 from T3 and T6 from T1 */
	fraction[5][i][bl][*z] =
	  (fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[T6T1][i][bl][*z] / multiple[scale_index[T6T1][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 16:			/* T5 from T3 and T6 from T2 */
	fraction[5][i][bl][*z] =
	  (fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[2][i][bl][*z] / multiple[scale_index[2][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 17:			/* T5 from T3 and T6 from T4 */
	fraction[5][i][bl][*z] =
	  (fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[4][i][bl][*z] / multiple[scale_index[4][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      case 18:			/* T5 and T6 from T3 */
	fraction[5][i][bl][*z] =
	  (fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
	  * multiple[scale_index[5][x][i]];
	fraction[6][i][bl][*z] =
	  (fraction[3][i][bl][*z] / multiple[scale_index[3][x][i]])
	  * multiple[scale_index[6][x][i]];
	break;
      }
    }
  }				/* for sblimit */
}

void II_dequantize_sample_aug (unsigned int sample[7][3][SBLIMIT],
			       unsigned int bit_alloc[7][SBLIMIT],
			       double fraction[7][SBLIMIT][3][12],
			       frame_params * fr_ps, int *z)
{
  int i, j, k, x, sbgr, l;
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
      for (k = 5; k < 7; k++)
	if (bit_alloc[k][i]) {
	  if ((fr_ps->header->dyn_cross_aug_mode[sbgr] == 0) ||
	      (fr_ps->header->dyn_cross_aug_mode[sbgr] == 1 && k == 5) ||
	      (fr_ps->header->dyn_cross_aug_mode[sbgr] == 2 && k == 5) ||
	      (fr_ps->header->dyn_cross_aug_mode[sbgr] == 3 && k == 5) ||
	      (fr_ps->header->dyn_cross_aug_mode[sbgr] == 4 && k == 5) ||
	      (fr_ps->header->dyn_cross_aug_mode[sbgr] == 5 && k == 6) ||
	      (fr_ps->header->dyn_cross_aug_mode[sbgr] == 10 && k == 6) ||
	      (fr_ps->header->dyn_cross_aug_mode[sbgr] == 14 && k == 6)) {
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
	  }			/* end if 2-channel dyn-cross mode */
	} /* if bit_alloc */
	else
	  fraction[k][i][j][*z] = 0.0;
  }
  for (i = sblimit; i < SBLIMIT; i++)
    for (j = 0; j < 3; j++)
      for (k = 5; k < 7; k++)
	fraction[k][i][j][*z] = 0.0;
}

void II_buffer_sample_aug (Bit_stream * bs,
			   frame_params * fr_ps,
			   unsigned int sample[7][3][SBLIMIT],
			   unsigned int bit_alloc[7][SBLIMIT], int gr)
{
  int i, j, k, m, sbgr, l;
  unsigned int nlevels, c = 0;
  int sblimit = fr_ps->sblimit_mc;
  int transmitted;
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

    for (j = 5; j < 7; j++) {
      if (bit_alloc[j][i]) {
	transmitted = 1;
	if (fr_ps->header->aug_dyn_cross_on == 1) {
	  if (j == 5) {
	    if (fr_ps->header->dyn_cross_aug_mode[sbgr] > 4)
	      transmitted = 0;
	  } else {
	    transmitted = 0;
	    switch (fr_ps->header->dyn_cross_aug_mode[sbgr]) {
	    case 0:
	    case 5:
	    case 10:
	    case 14:
	      transmitted = 1;
	      break;
	    }
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
    for (j = 5; j < 7; j++)
      for (k = 0; k < 3; k++)
	sample[j][k][i] = 0;
}

void II_decode_bitalloc_aug (Bit_stream * bs,
			     frame_params * fr_ps,
			     unsigned int bit_alloc[7][SBLIMIT],
			     int *l, int *m, int bits_log)
{
  layer *info = fr_ps->header;
  int i, j, c, sbgr;
  /* int stereo = fr_ps->stereo; *//* not used for mc - decoding */
  int sblimit = fr_ps->sblimit_mc;
  al_table *alloc = fr_ps->alloc_mc;
  unsigned int actual_alloc[7][SBLIMIT];

  for (i = 0; i < SBLIMIT; i++)
    for (j = *l; j < *m; j++)
      actual_alloc[j][i] = 0;

  for (i = 0; i < sblimit; i++)
    if (info->aug_dyn_cross_on == 0)
      for (j = *l; j < *m; j++)
	actual_alloc[j][i] = bit_alloc[j][i] =
	  (char) getbits (bs, (*alloc)[i][0].bits);
    else {			/* dyn. cross mode */

      int T5T0 = 0, T6T0 = 0, T6T1 = 1;

      if (i == 0)
	sbgr = 0;
      else
	for (c = 1; c < 12; c++) {
	  if ((sb_groups[c - 1] < i) && (i <= sb_groups[c])) {
	    sbgr = c;		/* search the valid subband group */
	    break;
	  }
	}

      if (info->tc_aug_alloc[sbgr] == 4 || info->tc_aug_alloc[sbgr] == 5)
	if (info->dyn_cross_LR == 0)
	  T6T1 = 0;
	else
	  T6T0 = 1;
      else if (info->tc_aug_alloc[sbgr] == 6 || info->tc_aug_alloc[sbgr] == 7)
	if (info->dyn_cross_LR)
	  T5T0 = 1;

      /* read bitalloc info from bitstream */
      switch (info->dyn_cross_aug_mode[sbgr]) {
      case 0:			/* T5 and T6 contains bitalloc info */
	actual_alloc[5][i] = bit_alloc[5][i] =
	  (char) getbits (bs, (*alloc)[i][0].bits);
	actual_alloc[6][i] = bit_alloc[6][i] =
	  (char) getbits (bs, (*alloc)[i][0].bits);
	break;
      case 1:			/* T5 contains bitalloc info */
      case 2:
      case 3:
      case 4:
	actual_alloc[5][i] = bit_alloc[5][i] =
	  (char) getbits (bs, (*alloc)[i][0].bits);
	break;

      case 5:			/* T6 contains bitalloc info */
      case 10:
      case 14:
	actual_alloc[6][i] = bit_alloc[6][i] =
	  (char) getbits (bs, (*alloc)[i][0].bits);
	break;
      }

      /* copy bitalloc info from other channels */
      switch (info->dyn_cross_aug_mode[sbgr]) {
      case 1:			/* copy T6 from T1 */
	bit_alloc[6][i] = bit_alloc[T6T1][i];
	break;
      case 2:			/* copy T6 from T2 */
	bit_alloc[6][i] = bit_alloc[2][i];
	break;
      case 3:			/* copy T6 from T4 */
	bit_alloc[6][i] = bit_alloc[4][i];
	break;
      case 4:			/* copy T6 from T5 */
	bit_alloc[6][i] = bit_alloc[5][i];
	break;
      case 5:			/* copy T5 from T0 */
	bit_alloc[5][i] = bit_alloc[T5T0][i];
	break;
      case 6:			/* copy T5 from T0 and T6 from T1 */
	bit_alloc[5][i] = bit_alloc[T5T0][i];
	bit_alloc[6][i] = bit_alloc[T6T1][i];
	break;
      case 7:			/* copy T5 from T0 and T6 from T2 */
	bit_alloc[5][i] = bit_alloc[T5T0][i];
	bit_alloc[6][i] = bit_alloc[2][i];
	break;
      case 8:			/* copy T5 from T0 and T6 from T4 */
	bit_alloc[5][i] = bit_alloc[T5T0][i];
	bit_alloc[6][i] = bit_alloc[4][i];
	break;
      case 9:			/* copy T5 and T6 from T0 */
	bit_alloc[5][i] = bit_alloc[T5T0][i];
	bit_alloc[6][i] = bit_alloc[T6T0][i];
	break;
      case 10:			/* copy T5 from T2 */
	bit_alloc[5][i] = bit_alloc[2][i];
	break;
      case 11:			/* copy T5 from T2 and T6 from T1 */
	bit_alloc[5][i] = bit_alloc[2][i];
	bit_alloc[6][i] = bit_alloc[T6T1][i];
	break;
      case 12:			/* copy T5 and T6 from T2 */
	bit_alloc[5][i] = bit_alloc[6][i] = bit_alloc[2][i];
	break;
      case 13:			/* copy T5 from T2 and T6 from T4 */
	bit_alloc[5][i] = bit_alloc[2][i];
	bit_alloc[6][i] = bit_alloc[4][i];
	break;
      case 14:			/* copy T5 from T3 */
	bit_alloc[5][i] = bit_alloc[3][i];
	break;
      case 15:			/* copy T5 from T3 and T6 from T1 */
	bit_alloc[5][i] = bit_alloc[3][i];
	bit_alloc[6][i] = bit_alloc[T6T1][i];
	break;
      case 16:			/* copy T5 from T3 and T6 from T2 */
	bit_alloc[5][i] = bit_alloc[3][i];
	bit_alloc[6][i] = bit_alloc[2][i];
	break;
      case 17:			/* copy T5 from T3 and T6 from T4 */
	bit_alloc[5][i] = bit_alloc[3][i];
	bit_alloc[6][i] = bit_alloc[4][i];
	break;
      case 18:			/* copy T5 and T6 from T3 */
	bit_alloc[5][i] = bit_alloc[6][i] = bit_alloc[3][i];
	break;
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

void mc_aug_composite_status_info (Bit_stream * bs, frame_params * fr_ps,
				   int tca_log, int dynx_log)
{
  layer *hdr = fr_ps->header;
  int sbgr, j, pci;

  hdr->aug_mtx_proc = getbits (bs, 2);
  hdr->aug_dyn_cross_on = get1bit (bs);
  hdr->aug_future_ext = get1bit (bs);

  if (hdr->aug_mtx_proc == 0) {
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->tc_aug_alloc[sbgr] = getbits (bs, 3);
  } else if (hdr->aug_mtx_proc == 1) {
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->tc_aug_alloc[sbgr] = getbits (bs, 2);
  } else
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->tc_aug_alloc[sbgr] = 0;

  if (tca_log)
    for (sbgr = 0; sbgr < 12; sbgr++)
      printf ("tc_aug_alloc[ %2d ] = %2d\n", sbgr, hdr->tc_aug_alloc[sbgr]);

  if (hdr->aug_dyn_cross_on == 1) {
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->dyn_cross_aug_mode[sbgr] = getbits (bs, 5);
  } else
    for (sbgr = 0; sbgr < 12; sbgr++)
      hdr->dyn_cross_aug_mode[sbgr] = 0;

  if (dynx_log)
    for (sbgr = 0; sbgr < 12; sbgr++)
      printf ("dynx_aug_mod[ %2d ] = %2d\n", sbgr,
	      hdr->dyn_cross_aug_mode[sbgr]);
}

void dematricing_aug (double pcm_sample[7][SBLIMIT][3][12],
		      frame_params * fr_ps)
{
  double c0, c1;		/* denormalization factors */
  int i, j, jj, k, tc_aug_alloc, l, sbgr = 0;
  layer *info = fr_ps->header;
  double tmp_sample1, tmp_sample2;

  c0 = 1.0 / 3.0;
  if (info->dematrix_procedure != 3)
    c0 *= sqrt (0.5);
  c1 = 1.0 / c0;

  for (jj = 0; jj < 12; jj++)
    for (j = 0; j < 3; ++j) {
      for (k = 0; k < SBLIMIT; k++) {
	if (k == 0)
	  sbgr = 0;
	else {
	  for (l = 1; l < 12; l++) {
	    if ((sb_groups[l - 1] < k) && (k <= sb_groups[l])) {
	      sbgr = l;		/* search the valid subband group */
	      break;
	    }
	  }
	}
	tc_aug_alloc = fr_ps->header->tc_aug_alloc[sbgr];

	if (info->aug_mtx_proc == 0)
	  switch (tc_aug_alloc) {
	  case 0:
	    pcm_sample[0][k][j][jj] -= pcm_sample[5][k][j][jj];
	    pcm_sample[1][k][j][jj] -= pcm_sample[6][k][j][jj];
	    pcm_sample[2][k][j][jj] -=
	      c0 * (pcm_sample[5][k][j][jj] + pcm_sample[6][k][j][jj]);
	    break;
	  case 1:
	    tmp_sample1 = pcm_sample[0][k][j][jj];
	    pcm_sample[0][k][j][jj] = pcm_sample[5][k][j][jj];
	    pcm_sample[5][k][j][jj] = tmp_sample1 - pcm_sample[5][k][j][jj];
	    pcm_sample[1][k][j][jj] -= pcm_sample[6][k][j][jj];
	    pcm_sample[2][k][j][jj] -=
	      c0 * (pcm_sample[5][k][j][jj] + pcm_sample[6][k][j][jj]);
	    break;
	  case 2:
	    pcm_sample[0][k][j][jj] -= pcm_sample[5][k][j][jj];
	    tmp_sample2 = pcm_sample[1][k][j][jj];
	    pcm_sample[1][k][j][jj] = pcm_sample[6][k][j][jj];
	    pcm_sample[6][k][j][jj] = tmp_sample2 - pcm_sample[6][k][j][jj];
	    pcm_sample[2][k][j][jj] -=
	      c0 * (pcm_sample[5][k][j][jj] + pcm_sample[6][k][j][jj]);
	    break;
	  case 3:
	    tmp_sample1 = pcm_sample[0][k][j][jj];
	    pcm_sample[0][k][j][jj] = pcm_sample[5][k][j][jj];
	    pcm_sample[5][k][j][jj] = tmp_sample1 - pcm_sample[5][k][j][jj];
	    tmp_sample2 = pcm_sample[1][k][j][jj];
	    pcm_sample[1][k][j][jj] = pcm_sample[6][k][j][jj];
	    pcm_sample[6][k][j][jj] = tmp_sample2 - pcm_sample[6][k][j][jj];
	    pcm_sample[2][k][j][jj] -=
	      c0 * (pcm_sample[5][k][j][jj] + pcm_sample[6][k][j][jj]);
	    break;
	  case 4:
	    pcm_sample[0][k][j][jj] -= pcm_sample[5][k][j][jj];
	    tmp_sample1 = pcm_sample[2][k][j][jj];
	    tmp_sample2 = pcm_sample[6][k][j][jj];
	    pcm_sample[2][k][j][jj] = tmp_sample2;
	    pcm_sample[6][k][j][jj] =
	      c1 * (tmp_sample1 - tmp_sample2) - pcm_sample[5][k][j][jj];
	    pcm_sample[1][k][j][jj] -= pcm_sample[6][k][j][jj];
	    break;
	  case 5:
	    tmp_sample1 = pcm_sample[0][k][j][jj];
	    pcm_sample[0][k][j][jj] = pcm_sample[5][k][j][jj];
	    pcm_sample[5][k][j][jj] = tmp_sample1 - pcm_sample[5][k][j][jj];
	    tmp_sample1 = pcm_sample[2][k][j][jj];
	    tmp_sample2 = pcm_sample[6][k][j][jj];
	    pcm_sample[2][k][j][jj] = tmp_sample2;
	    pcm_sample[6][k][j][jj] =
	      c1 * (tmp_sample1 - tmp_sample2) - pcm_sample[5][k][j][jj];
	    pcm_sample[1][k][j][jj] -= pcm_sample[6][k][j][jj];
	    break;
	  case 6:
	    pcm_sample[1][k][j][jj] -= pcm_sample[6][k][j][jj];
	    tmp_sample1 = pcm_sample[2][k][j][jj];
	    tmp_sample2 = pcm_sample[5][k][j][jj];
	    pcm_sample[2][k][j][jj] = tmp_sample2;
	    pcm_sample[5][k][j][jj] =
	      c1 * (tmp_sample1 - tmp_sample2) - pcm_sample[6][k][j][jj];
	    pcm_sample[0][k][j][jj] -= pcm_sample[5][k][j][jj];
	    break;
	  case 7:
	    tmp_sample1 = pcm_sample[1][k][j][jj];
	    pcm_sample[1][k][j][jj] = pcm_sample[6][k][j][jj];
	    pcm_sample[6][k][j][jj] = tmp_sample1 - pcm_sample[6][k][j][jj];
	    tmp_sample1 = pcm_sample[2][k][j][jj];
	    tmp_sample2 = pcm_sample[5][k][j][jj];
	    pcm_sample[2][k][j][jj] = tmp_sample2;
	    pcm_sample[5][k][j][jj] =
	      c1 * (tmp_sample1 - tmp_sample2) - pcm_sample[6][k][j][jj];
	    pcm_sample[0][k][j][jj] -= pcm_sample[5][k][j][jj];
	    break;
	} else if (info->aug_mtx_proc == 1)
	  switch (tc_aug_alloc) {
	  case 0:
	    pcm_sample[0][k][j][jj] -= pcm_sample[5][k][j][jj];
	    pcm_sample[1][k][j][jj] -= pcm_sample[6][k][j][jj];
	    break;
	  case 1:
	    tmp_sample1 = pcm_sample[0][k][j][jj];
	    pcm_sample[0][k][j][jj] = pcm_sample[5][k][j][jj];
	    pcm_sample[5][k][j][jj] = tmp_sample1 - pcm_sample[5][k][j][jj];
	    pcm_sample[1][k][j][jj] -= pcm_sample[6][k][j][jj];
	    break;
	  case 2:
	    pcm_sample[0][k][j][jj] -= pcm_sample[5][k][j][jj];
	    tmp_sample2 = pcm_sample[1][k][j][jj];
	    pcm_sample[1][k][j][jj] = pcm_sample[6][k][j][jj];
	    pcm_sample[6][k][j][jj] = tmp_sample2 - pcm_sample[6][k][j][jj];
	    break;
	  case 3:
	    tmp_sample1 = pcm_sample[0][k][j][jj];
	    pcm_sample[0][k][j][jj] = pcm_sample[5][k][j][jj];
	    pcm_sample[5][k][j][jj] = tmp_sample1 - pcm_sample[5][k][j][jj];
	    tmp_sample2 = pcm_sample[1][k][j][jj];
	    pcm_sample[1][k][j][jj] = pcm_sample[6][k][j][jj];
	    pcm_sample[6][k][j][jj] = tmp_sample2 - pcm_sample[6][k][j][jj];
	    break;
	  }
      }				/* for k < sblimit loop */
    }				/* for j < 3 loop */
}


void denormalizing_aug (double pcm_sample[7][SBLIMIT][3][12],
			frame_params * fr_ps)
{
  double c[7], d, denorm;	/* denormalization factors */
  int j, jj, k, l;
  layer *info = fr_ps->header;

  for (l = 0; l < 7; l++)
    c[l] = 1.0;

  switch (info->dematrix_procedure) {
    /* factors according to International Standard */
  case 0:
  case 2:
    c[2] = c[3] = c[4] = sqrt (2.0);	/* unweigh factor for C, Ls and Rs */
    break;
  case 1:
    c[2] = sqrt (2.0);		/* unweigh factor for C            */
    c[3] = c[4] = 2.0;		/* unweigh factor for Ls, Rs       */
    break;
  }

  if (info->aug_mtx_proc == 0)
    /* factors according to 7-ch augmentation */
    c[5] = c[6] = 4.0 / 3;	/* unweigh factor for LC, RC */

  /* denormalization factor */
  switch (info->dematrix_procedure * 10 + info->aug_mtx_proc) {
  case 00:
  case 20:
    denorm = 1.75 + 1.25 * sqrt (2.0);
    break;
  case 10:
    denorm = 2.25 + 0.75 * sqrt (2.0);
    break;
  case 30:
    denorm = 1.75;
    break;
  case 01:
  case 21:
    denorm = 2.0 + sqrt (2.0);
    break;
  case 11:
    denorm = 2.5 + 0.5 * sqrt (2.0);
    break;
  case 31:
    denorm = 2.0;
    break;
  case 03:
  case 23:
    denorm = 1.0 + sqrt (2.0);
    break;
  case 13:
    denorm = 1.5 + 0.5 * sqrt (2.0);
    break;
  case 33:
    denorm = 1.0;
    break;
  }

  for (l = 0; l < 7; l++)
    c[l] *= denorm;

  /* denormalizing */
  if (fr_ps->header->dematrix_procedure != 3
      || fr_ps->header->aug_mtx_proc != 3)
    for (jj = 0; jj < 12; jj++)
      for (j = 0; j < 3; ++j)
	for (k = 0; k < SBLIMIT; k++)
	  for (l = 0; l < 7; l++)
	    pcm_sample[l][k][j][jj] *= c[l];
}
#endif
