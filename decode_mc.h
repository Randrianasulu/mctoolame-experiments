
void mc_header (Bit_stream * bs, frame_params * fr_ps);
void mc_composite_status_info (Bit_stream * bs,
			       frame_params * fr_ps, int tca_log, int dynx_log);
void II_decode_bitalloc_mc (Bit_stream * bs,
			    frame_params * fr_ps,
			    unsigned int bit_alloc[7][SBLIMIT],
			    int *l, int *m, int bits_log);
void II_buffer_sample_mc (Bit_stream * bs,
			  frame_params * fr_ps,
			  unsigned int sample[7][3][SBLIMIT],
			  unsigned int bit_alloc[7][SBLIMIT],
			  int ch_start, int channels, int gr);
void II_dequantize_sample_mc (unsigned int sample[7][3][SBLIMIT],
			      unsigned int bit_alloc[7][SBLIMIT],
			      double fraction[7][SBLIMIT][3][12],
			      frame_params * fr_ps, int ch_start, int channels,
			      int *z);
void II_denormalize_sample_mc (double fraction[7][SBLIMIT][3][12],
			       unsigned int scale_index[7][3][SBLIMIT],
			       frame_params * fr_ps,
			       int x, int ch_start, int channels, int *z);

void dematricing_mc (double pcm_sample[7][SBLIMIT][3][12],
		     frame_params * fr_ps, double pred_buf[2][8][36 + PREDDEL]);

void denormalizing_mc (double pcm_sample[7][SBLIMIT][3][12],
		       frame_params * fr_ps, int channels);
double predict (double pred_buf[2][8][36 + PREDDEL],
		       frame_params * fr_ps,
		       int sb, int jj, int j, int ch, int idx);
