void out_fifo (int32_t pcm_sample[7][3][SBLIMIT],
	       int num,
	       frame_params * fr_ps,
	       int done, FILE * outFile, uint32_t *psampFrames, int ch, int wavedump);

void init_audio_outputs(int numchan);
void deinit_audio_outputs(void);
void out_fifo_new (int32_t pcm_sample[7][3][SBLIMIT],
	       int num,
	       frame_params * fr_ps,
		   int done, FILE * outFile, uint32_t *psampFrames, int numch);

void out_fifo_ml (int32_t pcm_sample[7][3][SBLIMIT],
		  int num,
		  frame_params * fr_ps,
		  int done, FILE * outFile, uint32_t *psampFrames);
