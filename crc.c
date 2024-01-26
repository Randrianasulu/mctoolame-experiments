#include "common.h"
#include "decoder.h"
#include "audio_write.h"
#include "crc.h"

void buffer_CRC (Bit_stream * bs, unsigned int *old_crc)
{
  *old_crc = getbits (bs, 16);
}

void recover_CRC_error (int32_t pcm_sample[7][3][SBLIMIT],
			int error_count,
			frame_params * fr_ps,
			FILE * outFile, uint32_t *psampFrames, int ch)
{
  int stereo = fr_ps->stereo;
  int num, done, i;
  int samplesPerFrame, samplesPerSlot;
  layer *hdr = fr_ps->header;
  long offset;
  short *temp;

  num = 3;
  if (hdr->lay == 1)
    num = 1;

  samplesPerSlot = SBLIMIT * num * stereo;
  samplesPerFrame = samplesPerSlot * 32;

  if (error_count == 1) {	/* replicate previous error_free frame */
    done = 1;
    /* flush out fifo */
    out_fifo (pcm_sample, num, fr_ps, done, outFile, psampFrames, ch, 0);
    /* go back to the beginning of the previous frame */
    offset = sizeof (short int) * samplesPerFrame;
    fseek (outFile, -offset, SEEK_CUR);
    done = 0;
    for (i = 0; i < 12; i++) {
      fread (pcm_sample, 2, samplesPerSlot, outFile);
      out_fifo (pcm_sample, num, fr_ps, done, outFile, psampFrames, ch, 0);
    }
  } else {			/* mute the frame */
    temp = (short *) pcm_sample;
    done = 0;
    for (i = 0; i < 2 * 3 * SBLIMIT; i++)
      *temp++ = MUTE;		/* MUTE value is in decoder.h */
    for (i = 0; i < 12; i++)
      out_fifo (pcm_sample, num, fr_ps, done, outFile, psampFrames, ch, 0);
  }
}
