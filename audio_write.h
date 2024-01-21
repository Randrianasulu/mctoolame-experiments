void out_fifo (long pcm_sample[7][3][SBLIMIT],
	       int num,
	       frame_params * fr_ps,
	       int done, FILE * outFile, unsigned long *psampFrames, int ch);

void init_audio_outputs(int numchan);
void deinit_audio_outputs(void);
void out_fifo_new (long pcm_sample[7][3][SBLIMIT],
	       int num,
	       frame_params * fr_ps,
		   int done, FILE * outFile, unsigned long *psampFrames, int numch);

void out_fifo_ml (long pcm_sample[7][3][SBLIMIT],
		  int num,
		  frame_params * fr_ps,
		  int done, FILE * outFile, unsigned long *psampFrames);
