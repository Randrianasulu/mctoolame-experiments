#include "common.h"
#include "audio_write.h"
#include <stdint.h>

//! Byte swap short
static int16_t swap_int16( int16_t val ) 
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

void out_fifo (int32_t pcm_sample[7][3][SBLIMIT],
	       int num,
	       frame_params * fr_ps,
	       int done, FILE * outFile, uint32_t *psampFrames, int ch, int wavedump)
{
  int i, j, l;
  static int16_t outsamp[1600];
  static long k = 0;

  //fprintf(stdout,"outfifo: num %i psampframes %i ch %i\n",num, *psampFrames, ch);
#define MIKESDODGYWAVEOUTPUT
#ifdef MIKESDODGYWAVEOUTPUT
  /* Dump seperate wave files for each channel */
  
if (wavedump)
  out_fifo_new (pcm_sample, 3, fr_ps, done, outFile, psampFrames,  ch);
#endif

  if (!done)
    for (i = 0; i < num; i++)
      for (j = 0; j < SBLIMIT; j++) {
	(*psampFrames)++;
	for (l = 0; l < ch; l++) {
	  if (!(k % 1600) && k) {
	    fwrite (outsamp, 2, 1600, outFile);
	    k = 0;
	  }
	  outsamp[k++] = swap_int16(pcm_sample[l][i][j]); // swap output samples, aiff on litte endian
	}
  } else if (k > 0) {
    fwrite (outsamp, 2, (int) k, outFile);
    k = 0;
  }
}

#define MAXCHANNELS 14
#define MTYPES 3
char soundfile[MTYPES][MAXCHANNELS][256] = {
  {"left.wav", "right.wav", "centre.wav", "left_surround.wav", "right_surround.wav"},
  {"left.wav", "right.wav", "centre.wav", "lfe_this_may_not_play.wav", "left_surround.wav", "right_surround.wav"},
  {"left.wav", "right.wav", "rearleft.wav", "rearright.wav"}
};
FILE *audioout[MAXCHANNELS];
int numchannels;
  /*********************************/
  /* Wave File Headers:   (Dec)    */
  /* 8-11 = "WAVE"                 */
  /* 22 = Stereo / Mono            */
  /*       01 = mono, 02 = stereo  */
  /* 24 = Sampling Frequency       */
  /* 32 = Data Rate                */
  /*       01 = x1 (8bit Mono)     */
  /*       02 = x2 (8bit Stereo or */
  /*                16bit Mono)    */
  /*       04 = x4 (16bit Stereo)  */
  /*********************************/
#define WAVEHEADERSIZE 44
// I'm going to lie about the size for the time being.
// Just write the header, and then go back and write the actual length at the end
int wave_header[WAVEHEADERSIZE] = {
  //0                     //total length
  //                       4     5     6     7
  0x52, 0x49, 0x46, 0x46, 0x24, 0x53, 0xff, 0xff, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
  //16                              //Mon//srate 24<<24,25<<16,26<<8,27
  //                                               24    25    26    27 
  0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0xBB, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00,
  //32                             // sound length 40    41    42    43
  //16bitmono
  0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0xff, 0xff, 0xff
};
uint32_t samples_written = 0;


void init_audio_outputs(int numchan) {
  int i,j;
  int type=-1;

  numchannels = numchan; // Set the global var for later on.
  if (numchan == 5)
    type = 0;
  if (numchan == 6)
    type = 1;
  if (numchan == 4)
    type = 2;
  if (soundfile>=0) {
    fprintf(stderr,"initialising %i output files\n",numchannels);
    for (i=0;i<numchannels;i++) {
      if ( (audioout[i] = fopen(soundfile[type][i], "w")) == NULL ) {
	fprintf(stderr,"Error opening %s for output\n",soundfile[type][i]);
	exit(99);
      }
      /* Write a really dodgy wave header */
      /* Fix this to write the proper sampling frequency */
      for (j=0;j<WAVEHEADERSIZE;j++)
	fputc(wave_header[j], audioout[i]);
    }
  }
}

void deinit_audio_outputs(void) {
  // Need to properly write the size of the wave file into the header
  // total length = sound length + 0x24 ?
  long int bytes_written = samples_written * 2;
  int i;
  //fprintf(stdout,"Total of %li samples written to each file\n", samples_written);
  //fprintf(stdout,"%li %li %li %li\n", (bytes_written & 0xff),(bytes_written>>8 & 0xff),
  //	  (bytes_written>>16 & 0xff),(bytes_written>>24 & 0xff) );
  for (i=0;i<numchannels;i++) {
    fseek(audioout[i], 40, SEEK_SET);
    fputc( (bytes_written & 0xff), audioout[i]);
    fputc( (bytes_written>>8 & 0xff), audioout[i]);
    fputc( (bytes_written>>16 & 0xff), audioout[i]);
    fputc( (bytes_written>>24 & 0xff), audioout[i]);
    fclose(audioout[i]);
  }
}

void out_fifo_new (int32_t pcm_sample[7][3][SBLIMIT],
	       int num,
	       frame_params * fr_ps,
	       int done, FILE * outFile, uint32_t *psampFrames, int numch)
{
  int gr, sb, ch;
  static int16_t outsamp[1600];
  static long k = 0;
  static int init=0;

  if (!init) {
    init_audio_outputs(numch);
    init++;
  }

  /* MFC March 2003. num should always be 3 */
  /* Done==FALSE except (a)last frame (b)crc error ? */

  for (ch=0; ch<numch; ch++) {
    k = 0;
    for (gr=0; gr<num; gr++) {
      for (sb=0;sb<SBLIMIT;sb++) {
	outsamp[k++] = pcm_sample[ch][gr][sb];
      }
    }
    fwrite(outsamp, 2, 96, audioout[ch]);
  }
  samples_written += 96;

}

void out_fifo_ml (int32_t pcm_sample[7][3][SBLIMIT],
		  int num,
		  frame_params * fr_ps,
		  int done, FILE * outFile, uint32_t *psampFrames)
{
  int i, j, l;
  int n_ml_ch = fr_ps->header->no_of_multi_lingual_ch;
  static int16_t outsamp[1600];
  static long k = 0;

  if (!done)
    for (i = 0; i < num; i++)
      for (j = 0; j < SBLIMIT; j++) {
	(*psampFrames)++;
	for (l = 0; l < n_ml_ch; l++) {
	  if (!(k % 1600) && k) {
	    fwrite (outsamp, 2, 1600, outFile);
	    k = 0;
	  }
	  outsamp[k++] = pcm_sample[l][i][j];
	}
  } else if (k > 0) {
    fwrite (outsamp, 2, (int) k, outFile);
    k = 0;
  }
}
