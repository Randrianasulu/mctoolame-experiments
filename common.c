#include "common.h"
#include "decoder.h"	/* 23/03/1995 JMZ Test_bit_stream */
#include <string.h>
#include <ctype.h>

#include "alloc_tables.h"

/***********************************************************************
*
*  Global Variable Definitions
*
***********************************************************************/
char *mode_names[4] = { "stereo", "j-stereo", "dual-ch", "single-ch" };
char *layer_names[3] = { "I", "II", "III" };

double s_freq[4] = { 44.1, 48, 32, 0 };

int bitrate[3][15] = {
  {1000, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
  {1000, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
  {1000, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
};

double multiple[64] = {
  2.00000000000000, 1.58740105196820, 1.25992104989487,
  1.00000000000000, 0.79370052598410, 0.62996052494744, 0.50000000000000,
  0.39685026299205, 0.31498026247372, 0.25000000000000, 0.19842513149602,
  0.15749013123686, 0.12500000000000, 0.09921256574801, 0.07874506561843,
  0.06250000000000, 0.04960628287401, 0.03937253280921, 0.03125000000000,
  0.02480314143700, 0.01968626640461, 0.01562500000000, 0.01240157071850,
  0.00984313320230, 0.00781250000000, 0.00620078535925, 0.00492156660115,
  0.00390625000000, 0.00310039267963, 0.00246078330058, 0.00195312500000,
  0.00155019633981, 0.00123039165029, 0.00097656250000, 0.00077509816991,
  0.00061519582514, 0.00048828125000, 0.00038754908495, 0.00030759791257,
  0.00024414062500, 0.00019377454248, 0.00015379895629, 0.00012207031250,
  0.00009688727124, 0.00007689947814, 0.00006103515625, 0.00004844363562,
  0.00003844973907, 0.00003051757813, 0.00002422181781, 0.00001922486954,
  0.00001525878906, 0.00001211090890, 0.00000961243477, 0.00000762939453,
  0.00000605545445, 0.00000480621738, 0.00000381469727, 0.00000302772723,
  0.00000240310869, 0.00000190734863, 0.00000151386361, 0.00000120155435,
  1E-20
};

int sb_groups[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 15, 26 };

/*int transmission_channel[8][5] = {{0, 1, 2, 3, 4},
				    {0, 1, 5, 3, 4},
				    {0, 1, 6, 3, 4},
				    {0, 1, 2, 5, 4},
				    {0, 1, 2, 3, 6},
				    {0, 1, 2, 5, 6},
				    {0, 1, 6, 5, 4},
				    {0, 1, 5, 3, 6}};
*/


/***********************************************************************
*
*  Global Function Definitions
*
***********************************************************************/


int js_bound (int lay, int m_ext)
{
  static int jsb_table[3][4] = { {4, 8, 12, 16}, {4, 8, 12, 16},
  {0, 4, 8, 16}
  };				/* lay+m_e -> jsbound */

  if (lay < 1 || lay > 3 || m_ext < 0 || m_ext > 3) {
    fprintf (stderr, "js_bound bad layer/modext (%d/%d)\n", lay, m_ext);
    exit (1);
  }
  return (jsb_table[lay - 1][m_ext]);
}

/* interpret data in hdr str to fields in fr_ps */
void hdr_to_frps (frame_params * fr_ps)
{
  layer *hdr = fr_ps->header;	/* (or pass in as arg?) */

  fr_ps->actual_mode = hdr->mode;
  if (hdr->mode != MPG_MD_NONE)
    fr_ps->stereo = (hdr->mode == MPG_MD_MONO) ? 1 : 2;
  else
    fr_ps->stereo = 0;

  Bitrate = bitrate[hdr->lay - 1][hdr->bitrate_index];
  S_freq = s_freq[hdr->sampling_frequency];

  if (hdr->lay == 2)
    fr_ps->sblimit = pick_table (fr_ps);
  else
    fr_ps->sblimit = SBLIMIT;

  if (hdr->mode == MPG_MD_JOINT_STEREO)
    fr_ps->jsbound = js_bound (hdr->lay, hdr->mode_ext);
  else
    fr_ps->jsbound = fr_ps->sblimit;
}

/* interpret data in hdr str to fields in fr_ps */
void mc_hdr_to_frps (frame_params * fr_ps)
{
  layer *hdr = fr_ps->header;	/* (or pass in as arg?) */

  if (fr_ps->header->surround == 3)	/* second stereo program */
    if (fr_ps->header->center == 1 || fr_ps->header->center == 3) {
      fr_ps->mc_channel = 3;	/* 3 + 2 */
      fr_ps->alloc_bits = 2;	/* 2 bits for tc_alloc */
      fr_ps->dyn_cross_bits = 1;
      fr_ps->pred_mode = 2;	/* FdB for prediction  3/0 */
    } else {
      fr_ps->mc_channel = 2;	/* 2 + 2 or 1 + 2 */
      fr_ps->alloc_bits = 0;	/* 0 bits for tc_alloc */
      fr_ps->dyn_cross_bits = 0;
      fr_ps->pred_mode = 5;	/* FdB for prediction  2/0 */
  } else if (fr_ps->header->surround == 2)	/* stereo surround */
    if (fr_ps->header->center == 1 || fr_ps->header->center == 3) {
      fr_ps->mc_channel = 3;	/* 3 mc-channels 3/2 */
      fr_ps->alloc_bits = 3;	/* 3 bits for tc_alloc */
      fr_ps->dyn_cross_bits = 4;
      fr_ps->pred_mode = 0;	/* 7/4/95 R.S. for prediction  3/2 */
    } else {
      fr_ps->mc_channel = 2;	/* 2 mc-channels 2/2 */
      fr_ps->alloc_bits = 2;	/* 2 bits for tc_alloc */
      fr_ps->dyn_cross_bits = 3;
      fr_ps->pred_mode = 3;	/* 7/4/95 R.S. for prediction  2/2 */
  } else if (fr_ps->header->surround == 1)	/* mono surround */
    if (fr_ps->header->center == 1 || fr_ps->header->center == 3) {
      fr_ps->mc_channel = 2;	/* 3/1 */
      fr_ps->alloc_bits = 3;	/* 3 bits for tc_alloc */
      fr_ps->dyn_cross_bits = 3;
      fr_ps->pred_mode = 1;	/* 7/4/95 R.S. for prediction 3/1 */
    } else {
      fr_ps->mc_channel = 1;	/* 2/1 */
      fr_ps->alloc_bits = 2;	/* 2 bits for tc_alloc */
      fr_ps->dyn_cross_bits = 1;
      fr_ps->pred_mode = 4;	/* 7/4/95 R.S. for prediction 2/1 */
  } else /* no surround */ if (fr_ps->header->center == 1
			       || fr_ps->header->center == 3) {
    fr_ps->mc_channel = 1;	/* 3/0 */
    fr_ps->alloc_bits = 2;	/* 2 bits for tc_alloc */
    fr_ps->dyn_cross_bits = 1;
    fr_ps->pred_mode = 2;	/* 3/0 */
  } else {
    fr_ps->mc_channel = 0;	/* 2/0 or 1/0 */
    fr_ps->alloc_bits = 0;	/* 2 bits for tc_alloc */
    fr_ps->dyn_cross_bits = 0;
    fr_ps->pred_mode = 5;	/* FdB for prediction  2/0 */
  }

  if ((hdr->no_of_multi_lingual_ch > 0) && (hdr->multi_lingual_layer > 0)) {
    fprintf (stderr, "MultiLingual not in Layer 2!\n exit.\n");
    exit (1);
  }

  if (fr_ps->mc_channel > 0 || hdr->lfe || hdr->no_of_multi_lingual_ch > 0)
    mc_pick_table (fr_ps);
}

void WriteHdr (frame_params * fr_ps, FILE * s)
{
  layer *info = fr_ps->header;

  fprintf (s, "HDR:  s=FFF, id=%X, l=%X, ep=%X, br=%X, sf=%X, pd=%X, ",
	   info->version, info->lay, !info->error_protection,
	   info->bitrate_index, info->sampling_frequency, info->padding);
  fprintf (s, "pr=%X, m=%X, js=%X, c=%X, o=%X, e=%X\n",
	   info->extension, info->mode, info->mode_ext,
	   info->copyright, info->original, info->emphasis);
  fprintf (s, "layer=%s, MPEG1-bitrate=%d, sfrq=%.1f, mode=%s, ",
	   layer_names[info->lay - 1], Bitrate =
	   bitrate[info->lay - 1][info->bitrate_index], S_freq =
	   s_freq[info->sampling_frequency], mode_names[info->mode]);
  fprintf (s, "sblim=%d, jsbd=%d, MPEG1-ch=%d\n", fr_ps->sblimit,
	   fr_ps->jsbound, fr_ps->stereo);
  fflush (s);
}

/*******************************************************************************
*
*  Allocate number of bytes of memory equal to "block".
*
*******************************************************************************/

void *mem_alloc (uint32_t block, char *item)
{
  void *ptr;

  ptr = (void *) malloc (block);
  if (ptr != NULL) {
    memset (ptr, 0, block);
  } else {
    fprintf (stderr, "Unable to allocate %s\n", item);
    exit (1);
  }

  return (ptr);

}

/****************************************************************************
*
*  Free memory pointed to by "*ptr_addr".
*
*****************************************************************************/

void mem_free (void **ptr_addr)
{

  if (*ptr_addr != NULL) {
    free (*ptr_addr);
    *ptr_addr = NULL;
  }

}

/****************************************************************************
*
*  Routines to convert between the Apple SANE extended floating point format
*  and the IEEE double precision floating point format.  These routines are
*  called from within the Audio Interchange File Format (AIFF) routines.
*
*****************************************************************************/

/*
*** Apple's 80-bit SANE extended has the following format:

 1       15      1            63
+-+-------------+-+-----------------------------+
|s|       e     |i|            f                |
+-+-------------+-+-----------------------------+
  msb        lsb   msb                       lsb

The value v of the number is determined by these fields as follows:
If 0 <= e < 32767,              then v = (-1)^s * 2^(e-16383) * (i.f).
If e == 32767 and f == 0,       then v = (-1)^s * (infinity), regardless of i.
If e == 32767 and f != 0,       then v is a NaN, regardless of i.

*** IEEE Draft Standard 754 Double Precision has the following format:

MSB
+-+---------+-----------------------------+
|1| 11 Bits |           52 Bits           |
+-+---------+-----------------------------+
 ^     ^                ^
 |     |                |
 Sign  Exponent         Mantissa
*/

/*****************************************************************************
*
*  double_to_extended()
*
*  Purpose:     Convert from IEEE double precision format to SANE extended
*               format.
*
*  Passed:      Pointer to the double precision number and a pointer to what
*               will hold the Apple SANE extended format value.
*
*  Outputs:     The SANE extended format pointer will be filled with the
*               converted value.
*
*  Returned:    Nothing.
*
*****************************************************************************/


void double_to_extended (double *pd, uint8_t ps[10])
{

#ifdef  MACINTOSH

  x96tox80 (pd, (extended *) ps);

#else

/* fixed bus alignment error, HP 27-may-93 */

  register uint32_t top2bits;

  register uint16_t *ps2;
  register IEEE_DBL *p_dbl;
  register SANE_EXT *p_ext;
  SANE_EXT ext_align;
  char *c_align;
  int i;

  p_dbl = (IEEE_DBL *) pd;
  p_ext = &ext_align;
  top2bits = p_dbl->hi & 0xc0000000L;
  p_ext->l1 = ((p_dbl->hi >> 4) & 0x3ff0000L) | top2bits;
  p_ext->l1 |= ((p_dbl->hi >> 5) & 0x7fffL) | 0x8000L;
  p_ext->l2 = (p_dbl->hi << 27) & 0xf8000000L;
  p_ext->l2 |= ((p_dbl->lo >> 5) & 0x07ffffffL);
  ps2 = (uint16_t *) &(p_dbl->lo);
  ps2++;
  p_ext->s1 = (*ps2 << 11) & 0xf800;

  c_align = (uint8_t *) p_ext;
  for (i = 0; i < 10; i++)
    ps[i] = c_align[i];

#endif

}


/*****************************************************************************
*
*  extended_to_double()
*
*  Purpose:     Convert from SANE extended format to IEEE double precision
*               format.
*
*  Passed:      Pointer to the Apple SANE extended format value and a pointer
*               to what will hold the the IEEE double precision number.
*
*  Outputs:     The IEEE double precision format pointer will be filled with
*               the converted value.
*
*  Returned:    Nothing.
*
*****************************************************************************/

void extended_to_double (uint8_t ps[10], double *pd)
{

#ifdef  MACINTOSH

  x80tox96 ((extended *) ps, pd);

#else

/* fixed bus alignment error, HP 27-may-93 */

  register uint32_t top2bits;

  register IEEE_DBL *p_dbl;
  register SANE_EXT *p_ext;
  SANE_EXT ext_align;
  char *c_align;
  int i;

  p_dbl = (IEEE_DBL *) pd;
  p_ext = &ext_align;

  c_align = (uint8_t *) p_ext;
  for (i = 0; i < 10; i++)
    c_align[i] = ps[i];

  top2bits = p_ext->l1 & 0xc0000000L;
  p_dbl->hi = ((p_ext->l1 << 4) & 0x3ff00000L) | top2bits;
  p_dbl->hi |= (p_ext->l1 << 5) & 0xffff0L;
  p_dbl->hi |= (p_ext->l2 >> 27) & 0x1f;
  p_dbl->lo = (p_ext->l2 << 5) & 0xffffffe0L;
  p_dbl->lo |= (uint32_t) ((p_ext->s1 >> 11) & 0x1f);

#endif

}

/*****************************************************************************
*
*  Read Audio Interchange File Format (AIFF) headers.
*
*****************************************************************************/

int aiff_read_headers (FILE * file_ptr,
		       IFF_AIFF * aiff_ptr, int *byte_per_sample)
{

  int i;
  //MFC register long seek_offset;

  uint8_t temp_sampleRate[10];
  char *dummy;
  //MFC char holder;
  Chunk FormChunk;
  CommonChunk CommChunk;
  SoundDataChunk SndDChunk;
  identifier ident;

  if (fseek (file_ptr, 0, SEEK_SET) != 0)
    return (-1);

  if (fread (&FormChunk, sizeof (Chunk), 1, file_ptr) != 1)
    return (-1);

#ifdef MSDOS

  holder = FormChunk.ckID[0];
  FormChunk.ckID[0] = FormChunk.ckID[3];
  FormChunk.ckID[3] = holder;
  holder = FormChunk.ckID[1];
  FormChunk.ckID[1] = FormChunk.ckID[2];
  FormChunk.ckID[2] = holder;

/* fixed bug in next line, HP 27-may-93 */
  holder = FormChunk.formType[0];
  FormChunk.formType[0] = FormChunk.formType[3];
  FormChunk.formType[3] = holder;
  holder = FormChunk.formType[1];
  FormChunk.formType[1] = FormChunk.formType[2];
  FormChunk.formType[2] = holder;

  FormChunk.ckSize = _lrotl (FormChunk.ckSize, 8);
#endif


/* fixed bug in next line, HP 27-may-93 */
  if (strncmp (FormChunk.ckID, IFF_ID_FORM, 4) != 0 ||
      strncmp (FormChunk.formType, IFF_ID_AIFF, 4) != 0)
    return (-1);		/* warning: different levels of indirection.7/8/92.sr */

  if (fread (&ident, sizeof (identifier), 1, file_ptr) != 1)
    return (-1);

#ifdef MSDOS

  holder = ident.name[0];
  ident.name[0] = ident.name[3];
  ident.name[3] = holder;
  holder = ident.name[1];
  ident.name[1] = ident.name[2];
  ident.name[2] = holder;

  ident.ck_length = _lrotl (ident.ck_length, 8);

#endif

/* fixed bug in next line, HP 27-may-93 */
  while (strncmp (ident.name, IFF_ID_COMM, 4) != 0) {
    dummy = calloc (ident.ck_length, sizeof (char));
/* changed "fread( &dummy," to "fread ( dummy,", HP 26-may-93 */
    if (fread (dummy, ident.ck_length, 1, file_ptr) != 1)
      return (-1);
    free (dummy);
/* fixed bug in next line, HP 27-may-93 */
    if (fread (&ident, sizeof (identifier), 1, file_ptr) != 1)
      return (-1);

#ifdef MSDOS

    holder = ident.name[0];
    ident.name[0] = ident.name[3];
    ident.name[3] = holder;
    holder = ident.name[1];
    ident.name[1] = ident.name[2];
    ident.name[2] = holder;

    ident.ck_length = _lrotl (ident.ck_length, 8);

#endif
  }

  for (i = 0; i < 4; ++i)
    CommChunk.ckID[i] = ident.name[i];

  CommChunk.ckSize = ident.ck_length;

  if (fread (&CommChunk.numChannels, sizeof (int16_t), 1, file_ptr) != 1)
    return (-1);

  if (fread (&CommChunk.numSampleFrames, sizeof (uint32_t), 1,
	     file_ptr) != 1)
    return (-1);

  if (fread (&CommChunk.sampleSize, sizeof (int16_t), 1, file_ptr) != 1)
    return (-1);

  if (fread (CommChunk.sampleRate, sizeof (uint8_t[10]), 1, file_ptr) != 1)
    return (-1);

#ifdef MSDOS

  CommChunk.sampleSize = _rotl (CommChunk.sampleSize, 8);
  CommChunk.ckSize = _lrotl (CommChunk.ckSize, 8);
  CommChunk.numChannels = _rotl (CommChunk.numChannels, 8);
  CommChunk.numSampleFrames = _lrotl (CommChunk.numSampleFrames, 8);

#endif


  *byte_per_sample = ceil ((double) CommChunk.sampleSize / 8);

  for (i = 0; i < sizeof (uint8_t[10]); i++)
    temp_sampleRate[i] = CommChunk.sampleRate[i];

  extended_to_double (temp_sampleRate, &aiff_ptr->sampleRate);


/* to start the search again from the beginning, HP 27-may-93 */
  fseek (file_ptr, sizeof (Chunk), SEEK_SET);

  if (fread (&ident, sizeof (identifier), 1, file_ptr) != 1)
    return (-1);

#ifdef MSDOS

  holder = ident.name[0];
  ident.name[0] = ident.name[3];
  ident.name[3] = holder;
  holder = ident.name[1];
  ident.name[1] = ident.name[2];
  ident.name[2] = holder;

  ident.ck_length = _lrotl (ident.ck_length, 8);

#endif

/* fixed bug in next line, HP 27-may-93 */
  while (strncmp (ident.name, IFF_ID_SSND, 4) != 0) {
    dummy = calloc (ident.ck_length, sizeof (char));
/* changed "fread( &dummy," to "fread ( dummy,", HP 26-may-93 */
    if (fread (dummy, ident.ck_length, 1, file_ptr) != 1)
      return (-1);
    free (dummy);
    if (fread (&ident, sizeof (identifier), 1, file_ptr) != 1)
      return (-1);
/* the following lines are not necessary, HP 27-may-93 */
/*
		  {
		  fseek(file_ptr, 0, SEEK_SET);
		  if(fread( &ident, sizeof(identifier), 1, file_ptr) != 1)
		  return(-1);
		  }
*/

#ifdef MSDOS


    holder = ident.name[0];
    ident.name[0] = ident.name[3];
    ident.name[3] = holder;
    holder = ident.name[1];
    ident.name[1] = ident.name[2];
    ident.name[2] = holder;

    ident.ck_length = _lrotl (ident.ck_length, 8);

#endif
  }

  for (i = 0; i < 4; ++i)
    SndDChunk.ckID[i] = ident.name[i];

  SndDChunk.ckSize = ident.ck_length;

  if (fread (&SndDChunk.offset, sizeof (uint32_t), 1, file_ptr) != 1)
    return (-1);

  if (fread (&SndDChunk.blockSize, sizeof (uint32_t), 1, file_ptr) != 1)
    return (-1);

#ifdef MSDOS

  SndDChunk.offset = _lrotl (SndDChunk.offset, 8);
  SndDChunk.blockSize = _lrotl (SndDChunk.blockSize, 8);

#endif

/* why seek behinde the SSND Chunk ????, HP 27-may-93 */
/*
   seek_offset = SndDChunk.ckSize - sizeof(SoundDataChunk) +
      sizeof(ChunkHeader);

   if (fseek(file_ptr, seek_offset, SEEK_CUR) != 0)
	  return(-1);
*/

  aiff_ptr->numChannels = CommChunk.numChannels;
  aiff_ptr->numSampleFrames = CommChunk.numSampleFrames;
  aiff_ptr->sampleSize = CommChunk.sampleSize;
  aiff_ptr->blkAlgn.offset = SndDChunk.offset;
  aiff_ptr->blkAlgn.blockSize = SndDChunk.blockSize;
  strncpy (aiff_ptr->sampleType, SndDChunk.ckID, 4);

  return (0);

}


/*****************************************************************************
*
*  Seek past some Audio Interchange File Format (AIFF) headers to sound data.
*
*****************************************************************************/

int aiff_seek_to_sound_data (FILE * file_ptr)
{

/*   if (fseek(file_ptr, sizeof(Chunk) + sizeof(SoundDataChunk), SEEK_SET) != 0) */
  if (fseek
      (file_ptr,
       sizeof (Chunk) + sizeof (CommonChunk) + sizeof (SoundDataChunk),
       SEEK_SET) != 0)
    return (-1);
  else
    return (0);

}

/*******************************************************************************
*
*  Write Audio Interchange File Format (AIFF) headers.
*
*******************************************************************************/

int aiff_write_headers (FILE * file_ptr, IFF_AIFF * aiff_ptr)
{

  int i;
  //MFC register long seek_offset;

  uint8_t temp_sampleRate[10];

  Chunk FormChunk;
  CommonChunk CommChunk;
  SoundDataChunk SndDChunk;


  strncpy (FormChunk.ckID, IFF_ID_FORM, 4);
  strncpy (FormChunk.formType, IFF_ID_AIFF, 4);
  strncpy (CommChunk.ckID, IFF_ID_COMM, 4);	/*7/7/93,SR,changed FormChunk to CommChunk */


  double_to_extended (&aiff_ptr->sampleRate, temp_sampleRate);

  for (i = 0; i < sizeof (uint8_t[10]); i++)
    CommChunk.sampleRate[i] = temp_sampleRate[i];

  CommChunk.numChannels = aiff_ptr->numChannels;
  CommChunk.numSampleFrames = aiff_ptr->numSampleFrames;
  CommChunk.sampleSize = aiff_ptr->sampleSize;
  SndDChunk.offset = aiff_ptr->blkAlgn.offset + 2;
  SndDChunk.blockSize = aiff_ptr->blkAlgn.blockSize;
  strncpy ( /*(unsigned long *) */ SndDChunk.ckID, aiff_ptr->sampleType, 4);

  CommChunk.ckSize = sizeof (CommChunk.numChannels) +
    sizeof (CommChunk.numSampleFrames) + sizeof (CommChunk.sampleSize) +
    sizeof (CommChunk.sampleRate);

  SndDChunk.ckSize = sizeof (SoundDataChunk) - sizeof (ChunkHeader) +
    (CommChunk.sampleSize + BITS_IN_A_BYTE - 1) / BITS_IN_A_BYTE *
    CommChunk.numChannels * CommChunk.numSampleFrames;

  FormChunk.ckSize = sizeof (Chunk) + SndDChunk.ckSize + sizeof (ChunkHeader) +
    CommChunk.ckSize;

  if (fseek (file_ptr, 0, SEEK_SET) != 0)
    return (-1);

  if (fwrite (&FormChunk, sizeof (Chunk), 1, file_ptr) != 1)
    return (-1);
/*
   if (fwrite(&SndDChunk, sizeof(SoundDataChunk), 1, file_ptr) != 1)
      return(-1);

   seek_offset = SndDChunk.ckSize - sizeof(SoundDataChunk) +
      sizeof(ChunkHeader);

   if (fseek(file_ptr, seek_offset, SEEK_CUR) != 0)
      return(-1);
*/

  if (fwrite (CommChunk.ckID, sizeof (ID), 1, file_ptr) != 1)
    return (-1);

  if (fwrite (&CommChunk.ckSize, sizeof (int32_t), 1, file_ptr) != 1)
    return (-1);

  if (fwrite (&CommChunk.numChannels, sizeof (int16_t), 1, file_ptr) != 1)
    return (-1);

  if (fwrite (&CommChunk.numSampleFrames, sizeof (uint32_t), 1,
	      file_ptr) != 1)
    return (-1);

  if (fwrite (&CommChunk.sampleSize, sizeof (int16_t), 1, file_ptr) != 1)
    return (-1);

  if (fwrite (CommChunk.sampleRate, sizeof (uint8_t[10]), 1, file_ptr) != 1)
    return (-1);

  /* 960815 FdB put the sound data chunk after the common chunk */
  if (fwrite (&SndDChunk, sizeof (SoundDataChunk), 1, file_ptr) != 1)
    return (-1);

  return (0);

}

/*****************************************************************************
*
*  bit_stream.c package
*  Author:  Jean-Georges Fritsch, C-Cube Microsystems
*
*****************************************************************************/

/********************************************************************
  This package provides functions to write (exclusive or read)
  information from (exclusive or to) the bit stream.

  If the bit stream is opened in read mode only the get functions are
  available. If the bit stream is opened in write mode only the put
  functions are available.
********************************************************************/

//static char *he = "0123456789ABCDEF";

/* open the device to read the bit stream from it */
int open_bit_stream_r (Bit_stream * bs,	/* bit stream structure */
		       char *bs_filenam,	/* name of the bit stream file */
		       int size /* size of the buffer */ )
{
  register uint32_t n;
  register int i = 0, j = 0;
  register unsigned char flag = 1;
  unsigned char val;

  if ((bs->pt = fopen (bs_filenam, "rb")) == NULL) {
    printf ("Could not find \"%s\".\n", bs_filenam);
    return (0);
  }
  fseek (bs->pt, bs->header_size, 0);

  do {
    n = fread (&val, sizeof (unsigned char), 1, bs->pt);
    switch (val) {
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
      j++;
    case 0xa:			/* \n */
      break;

    default:			/* detection of an binary character */
      flag--;
      i = 300;
      break;
    }

  } while (flag & n);

  if (flag) {
    printf ("the bit stream file %s is an ASCII file\n", bs_filenam);
    printf ("ASCII files are not supported in this version\n");
    exit (1);
    bs->format = ASCII;
  } else {
    bs->format = BINARY;
    printf ("the bit stream file %s is a BINARY file\n", bs_filenam);
  }

  fclose (bs->pt);

  if ((bs->pt = fopen (bs_filenam, "rb")) == NULL) {
    printf ("Could not find \"%s\".\n", bs_filenam);
    return (0);
  }
  fseek (bs->pt, bs->header_size, 0);

  bs->curpos = 0;
  bs->totbits = 0;
  bs->mode = READ_MODE;
  bs->eobs = FALSE;
  return (1);
}

/*close the device containing the bit stream after a read process*/
void close_bit_stream_r (Bit_stream * bs)
{
  fclose (bs->pt);
}

unsigned int get1bit (Bit_stream * bs)
{
  if (bs->curpos < bs->totbits) {
#ifdef	PrintBitDebug
    printf ("pos: %5d bits: %2d code: %4x val: %5d\n",
	    bs->curpos, 1, bs->bits[bs->curpos], bs->bits[bs->curpos]);
    fflush (stdout);
#endif
    return (bs->bits[bs->curpos++]);
  } else {
    fprintf (stderr, "get1bits: bs->curpos = %4li bs->totbits = %4li bits\n",
	     bs->curpos, bs->totbits);
    exit (1);
  }
}

uint32_t getbits (Bit_stream * bs, int n)
{
  unsigned long val;
  int i;

  if (bs->curpos + n <= bs->totbits) {
    for (i = val = 0; i < n; i++)
      val = 2 * val + bs->bits[bs->curpos++];

#ifdef	PrintBitDebug
    printf ("pos: %5d bits: %2d code: %4x val: %5d\n",
	    bs->curpos - n, n, val, val);
    fflush (stdout);
#endif

    return (val);
  } else {
    fprintf (stderr, "getbits: n: %4i bs->curpos: %4li bs->totbits = %4li bits\n",
	     n, bs->curpos, bs->totbits);
    exit (1);
  }
}

void program_information (void)
{
  printf ("ISO MPEG Audio Subgroup Software Simulation Group (1996)\n");
  printf ("ISO 13818-3 MPEG-2 Audio Multichannel Decoder\n");
  printf ("%s\n", VERSION);
}

/*******************************************************************/

/*return the status of the bit stream*/
/* returns 1 if end of bit stream was reached */
/* returns 0 if end of bit stream was not reached */
int end_bs (Bit_stream * bs)
{
  return (bs->eobs);
}

static void bytes_to_bits (char *w_code, unsigned char *in, int nbytes)
{
  int i, j, bpos, d;

  bpos = 0;
  for (i = 0; i < nbytes; i++) {
    d = in[i];
    for (j = 7; j >= 0; j--) {
      w_code[bpos + j] = d & 1;
      d = d / 2;
    }
    bpos += 8;
  }
}

/*this function seeks for a byte aligned sync word in the bit stream and
  places the bit stream pointer right after the sync.
  This function returns 1 if the sync was found otherwise it returns 0 */

int seek_sync_mpg (Bit_stream * bs)
{				/* bit stream structure */
  uint32_t val = 0;
  unsigned char byte;
  unsigned char bytes[2000];	/* bytes of an MPEG-1 frame */
  int32_t sync = SYNC_WORD;	/* sync word maximum 32 bits */
  int N = SYNC_WORD_LNGTH;	/* sync word length */
  int i, sync_bytes = N / 8;
  int version, layer, br_index, bit_rate, c, f_sampl=0, padding, slots, nbytes;

  if (fread (bytes, 1, 4, bs->pt) != 4) {
    printf ("next mpg header not found\n");
    return (0);
  }

  for (i = 0; i < sync_bytes; i++) {
    val <<= 8;
    val |= bytes[i];
  }
  if ((N % 8) != 0) {
    val <<= N % 8;
    byte = bytes[sync_bytes] >> (8 - (N % 8));
    val |= byte;
  }

  if (val != sync) {
    printf ("no mpg sync found: %4li\n", val);
    return (0);
  }

  bytes_to_bits (bs->bits, bytes, 4);

  version = bs->bits[12];
  if (version == 0) {
    fprintf (stderr, "Error: Illegal version bit.\n");
    exit (1);
  }
  layer = 4 - 2 * bs->bits[13] - bs->bits[14];
  for (i = 16, br_index = 0; i < 20; i++)
    br_index = 2 * br_index + bs->bits[i];
  if (br_index == 0 || br_index == 15) {
    fprintf (stderr, "Error: Illegal bit_rate index (0 or 15).\n");
    exit (1);
  }
  bit_rate = bitrate[layer - 1][br_index] * 1000;
  c = 2 * bs->bits[20] + bs->bits[21];
  switch (c) {
  case 0:
    f_sampl = 44100;
    break;
  case 1:
    f_sampl = 48000;
    break;
  case 2:
    f_sampl = 32000;
    break;
  case 3:
    fprintf (stderr, "Error: reserved sampling rate.\n");
    exit (1);
  }
  padding = bs->bits[22];

  if (layer == 1)
    slots = bit_rate * 12 / f_sampl;
  else
    slots = bit_rate * 144 / f_sampl;

  slots += padding;

  if (layer == 1)
    nbytes = slots * 4;
  else
    nbytes = slots;

  if (fread (&bytes[4], 1, nbytes - 4, bs->pt) != nbytes - 4) {
    printf ("no n-4 bytes\n");
    return (0);
  }

  bytes_to_bits (&bs->bits[32], &bytes[4], nbytes - 4);

  bs->curpos = 12;
  bs->totbits = nbytes * 8;
  bs->eobs = 0;

  return (1);
}

int seek_sync_ext (Bit_stream * bs, frame_params * fr_ps)
{				/* bit stream structure */
  layer *info = fr_ps->header;
  uint32_t val = 0;
  unsigned char byte;
  unsigned char bytes[2048];	/* bytes of an MPEG-2 extention frame */
  int32_t sync = EXT_SYNCWORD;	/* sync word maximum 32 bits */
  int N = SYNC_WORD_LNGTH;	/* sync word length */
  int i, sync_bytes = N / 8;
  int nbytes, lcrc, la_bytes, la_bits;

  if (fread (bytes, 1, 5, bs->pt) != 5) {
    printf ("next ext header not found\n");
    return (0);
  }

  for (i = 0; i < sync_bytes; i++) {
    val <<= 8;
    val |= bytes[i];
  }
  if ((N % 8) != 0) {
    val <<= N % 8;
    byte = bytes[sync_bytes] >> (8 - (N % 8));
    val |= byte;
  }

  if (val != sync) {
    printf ("no ext sync found: %4li\n", val);
    return (0);
  }

  nbytes = (((int) (bytes[3] & 0x0f) * 256) + bytes[4]) / 2;

  if (fread (&bytes[5], 1, nbytes - 5, bs->pt) != nbytes - 5)
    return (0);
  bytes_to_bits (bs->bits, bytes, nbytes);

  bs->totbits = nbytes * 8;
  bs->curpos = 12;
  bs->eobs = 0;

  info->ext_crc_check = getbits (bs, 16);
  info->ext_length = getbits (bs, 11);
  info->reserved_bit = get1bit (bs);

#ifdef EXTENSION_CRC_CHECK

  lcrc = info->ext_length * 8 - 28;
  if (lcrc > 128)
    lcrc = 128;
  la_bytes = (lcrc - 12) / 8;
  la_bits = (lcrc - 12) % 8;

  for (i = 0; i < la_bytes; i++)
    info->ext_crc_bits[i] = getbits (bs, 8);
  info->ext_crc_bits[la_bytes] = getbits (bs, la_bits);
  bs->curpos = 40;

  if (mc_ext_error_check (fr_ps, la_bytes, la_bits) == 0)
    return (0);
#endif

  return (1);
}

/*****************************************************************************
*
*  End of bit_stream.c package
*
*****************************************************************************/

/*****************************************************************************
*
*  CRC error protection package
*
*****************************************************************************/

void update_CRC (unsigned int data, unsigned int length, unsigned int *crc)
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking >>= 1)) {
    carry = *crc & 0x8000;
    *crc <<= 1;
    if (!carry ^ !(data & masking))
      *crc ^= CRC16_POLYNOMIAL;
  }
  *crc &= 0xffff;

#ifdef	PrintCRCDebug
  printf ("crc_len: %2d code: %4x crc: %4x\n", length, data, *crc);
  fflush (stdout);
#endif
}

void II_CRC_calc (frame_params * fr_ps,
		  unsigned int bit_alloc[7][SBLIMIT],
		  unsigned int scfsi[7][SBLIMIT], unsigned int *crc)
{
  int i, k;
  layer *info = fr_ps->header;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;

  *crc = 0xffff;		/* changed from '0' 92-08-11 shn */
  update_CRC (info->bitrate_index, 4, crc);
  update_CRC (info->sampling_frequency, 2, crc);
  update_CRC (info->padding, 1, crc);
  update_CRC (info->extension, 1, crc);
  update_CRC (info->mode, 2, crc);
  update_CRC (info->mode_ext, 2, crc);
  update_CRC (info->copyright, 1, crc);
  update_CRC (info->original, 1, crc);
  update_CRC (info->emphasis, 2, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < ((i < jsbound) ? stereo : 1); k++)
      update_CRC (bit_alloc[k][i], (*alloc)[i][0].bits, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < stereo; k++)
      if (bit_alloc[k][i])
	update_CRC (scfsi[k][i], 2, crc);
}


void mc_error_check (frame_params * fr_ps,
		     unsigned int bit_alloc[7][SBLIMIT],
		     unsigned int scfsi[7][SBLIMIT],
		     unsigned int *crc, int ch_start, int channels)
{
  int i, m, pci, c, sbgr=0;
  layer *info = fr_ps->header;
  int sblimit = fr_ps->sblimit_mc;
  al_table *alloc = fr_ps->alloc_mc;

  *crc = 0xffff;		/* changed from '0' 92-08-11 shn */
  update_CRC (info->ext_bit_stream_present, 1, crc);
  if (info->ext_bit_stream_present == 1)
    update_CRC (info->n_ad_bytes, 8, crc);

  update_CRC (info->center, 2, crc);
  update_CRC (info->surround, 2, crc);
  update_CRC (info->lfe, 1, crc);
  update_CRC (info->audio_mix, 1, crc);
  update_CRC (info->dematrix_procedure, 2, crc);
  update_CRC (info->no_of_multi_lingual_ch, 3, crc);
  update_CRC (info->multi_lingual_fs, 1, crc);
  update_CRC (info->multi_lingual_layer, 1, crc);
  update_CRC (info->copyright_ident_bit, 1, crc);
  update_CRC (info->copyright_ident_start, 1, crc);

  update_CRC (info->tc_sbgr_select, 1, crc);
  update_CRC (info->dyn_cross_on, 1, crc);
  update_CRC (info->mc_prediction_on, 1, crc);

  if (info->tc_sbgr_select == 1)
    update_CRC (info->tc_allocation, fr_ps->alloc_bits, crc);
  else
    for (i = 0; i < 12; i++)
      update_CRC (info->tc_alloc[i], fr_ps->alloc_bits, crc);

  if (info->dyn_cross_on == 1) {
    update_CRC (info->dyn_cross_LR, 1, crc);
    for (i = 0; i < 12; i++) {
      update_CRC (info->dyn_cross_mode[i], fr_ps->dyn_cross_bits, crc);
      if (info->surround == 3)
	update_CRC (info->dyn_second_stereo[i], 1, crc);
    }
  }

  if (info->mc_prediction_on == 1) {
    for (i = 0; i < 8; i++) {	/* new sbgr < 8 */
      update_CRC (info->mc_prediction[i], 1, crc);
      if (info->mc_prediction[i] == 1) {
	for (pci = 0;
	     pci < pred_coef_table[fr_ps->pred_mode][info->dyn_cross_mode[i]];
	     pci++)
	  update_CRC (info->mc_predsi[i][pci], 2, crc);
      }
    }
  }

  if (info->lfe)
    update_CRC (info->lfe_alloc, (*alloc)[0][0].bits, crc);

  for (i = 0; i < sblimit; i++)
    if (info->dyn_cross_on == 0)
      for (m = ch_start; m < channels; ++m) {
	if ((i < 12) || (m != 2) || (fr_ps->header->center != 3))
	  update_CRC (bit_alloc[m][i], (*alloc)[i][0].bits, crc);
	/* to limit the CRC calc., if the phant. center valid */
    } else {
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
	for (m = ch_start; m < channels; m++) {
	  if (( (info->center != 3)  ||  (i < 12)  ||  (m != 2)  )) {
	    if (info->surround == 3 && info->dyn_second_stereo[sbgr] == 1) {
	      if ((info->center == 0 || m != 4)
		  && (info->center != 0 || m != (ch_start + 1))) {
		update_CRC (bit_alloc[m][i], (*alloc)[i][0].bits, crc);
	      }
	    } else {
	      update_CRC (bit_alloc[m][i], (*alloc)[i][0].bits, crc);
	    }
	  }
	}
      } else if (fr_ps->dyn_cross_bits == 1) {	/* for channel mode 3/0 and 2/1 */
	if (info->surround == 3) {	/* 3/0 + 2/0 */
	  update_CRC (bit_alloc[3][i], (*alloc)[i][0].bits, crc);
	  if (info->dyn_second_stereo[sbgr] == 0)
	    update_CRC (bit_alloc[4][i], (*alloc)[i][0].bits, crc);
	}
      } else if (fr_ps->dyn_cross_bits == 3) {	/* for channel mode 3/1 and 2/2 */
	if ((info->center != 3) || (i < 12))	/* 3/2 + no phantom center */
	  if ((info->dyn_cross_mode[sbgr] == 1)
	      || (info->dyn_cross_mode[sbgr] == 4))
	    update_CRC (bit_alloc[2][i], (*alloc)[i][0].bits, crc);

	if (info->dyn_cross_mode[sbgr] == 2)
	  update_CRC (bit_alloc[3][i], (*alloc)[i][0].bits, crc);
      } else if (fr_ps->dyn_cross_bits == 4) {	/* for channel mode 3/2 */
	/* T2 */
	if ((info->center != 3) || (i < 12))	/* 3/2 + no phantom center */
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
	    update_CRC (bit_alloc[2][i], (*alloc)[i][0].bits, crc);
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
	  update_CRC (bit_alloc[3][i], (*alloc)[i][0].bits, crc);
	  break;
	}

	/* T4 */
	switch (info->dyn_cross_mode[sbgr]) {
	case 2:		/* T4 contains bitalloc info */
	case 3:
	case 6:
	case 9:
	  update_CRC (bit_alloc[4][i], (*alloc)[i][0].bits, crc);
	  break;
	}
      }
    }

  for (i = 0; i < sblimit; i++)
    for (m = ch_start; m < channels; m++)
      if (bit_alloc[m][i])
	update_CRC (scfsi[m][i], 2, crc);
}


#ifdef Augmentation_7ch
void mc_aug_error_check (frame_params * fr_ps,
			 unsigned int bit_alloc[7][SBLIMIT],
			 unsigned int scfsi[7][SBLIMIT], unsigned int *crc)
{
  int i, m, l, pci, c, sbgr;
  layer *info = fr_ps->header;
  int sblimit = fr_ps->sblimit_mc;
  al_table *alloc = fr_ps->alloc_mc;

  *crc = 0xffff;		/* changed from '0' 92-08-11 shn */

  update_CRC (info->aug_mtx_proc, 2, crc);
  update_CRC (info->aug_dyn_cross_on, 1, crc);
  update_CRC (info->aug_future_ext, 1, crc);

  if (info->aug_mtx_proc == 0)
    for (i = 0; i < 12; i++)
      update_CRC (info->tc_aug_alloc[i], 3, crc);
  else if (info->aug_mtx_proc == 1)
    for (i = 0; i < 12; i++)
      update_CRC (info->tc_aug_alloc[i], 2, crc);

  if (info->aug_dyn_cross_on == 1)
    for (i = 0; i < 12; i++)
      update_CRC (info->dyn_cross_aug_mode[i], 5, crc);

  for (i = 0; i < sblimit; i++) {
    if (i == 0)
      sbgr = 0;
    else
      for (c = 1; c < 12; c++) {
	if ((sb_groups[c - 1] < i) && (i <= sb_groups[c])) {
	  sbgr = c;		/* search the valid subband group */
	  break;
	}
      }

    /* check bitalloc info */
    switch (info->dyn_cross_aug_mode[sbgr]) {
    case 0:			/* T5 and T6 contains bitalloc info */
      update_CRC (bit_alloc[5][i], (*alloc)[i][0].bits, crc);
      update_CRC (bit_alloc[6][i], (*alloc)[i][0].bits, crc);
      break;

    case 1:			/* T5 contains bitalloc info */
    case 2:
    case 3:
    case 4:
      update_CRC (bit_alloc[5][i], (*alloc)[i][0].bits, crc);
      break;

    case 5:			/* T6 contains bitalloc info */
    case 10:
    case 14:
      update_CRC (bit_alloc[6][i], (*alloc)[i][0].bits, crc);
      break;
    }
  }

  for (i = 0; i < sblimit; i++)
    for (m = 5; m < 7; m++)
      if (bit_alloc[m][i])
	update_CRC (scfsi[m][i], 2, crc);
}
#endif


int mc_ext_error_check (frame_params * fr_ps, int bytes, int bits)
{
  layer *info = fr_ps->header;
  int i;
  unsigned int crc;

  crc = 0xffff;
  update_CRC (info->ext_length, 11, &crc);
  update_CRC (info->reserved_bit, 1, &crc);
  for (i = 0; i < bytes; i++)
    update_CRC (info->ext_crc_bits[i], 8, &crc);
  update_CRC (info->ext_crc_bits[bytes], bits, &crc);

  if (crc != info->ext_crc_check) {
    printf ("\nERROR in EXT.-CRC \n");
    return (0);
  } else
    return (1);
}

/*****************************************************************************
*
*  End of CRC error protection package
*
*****************************************************************************/
