
void II_denormalize_sample_aug (double fraction[7][SBLIMIT][3][12],
				unsigned int scale_index[7][3][SBLIMIT],
				frame_params * fr_ps, int x, int *z);

void II_dequantize_sample_aug (unsigned int sample[7][3][SBLIMIT],
			       unsigned int bit_alloc[7][SBLIMIT],
			       double fraction[7][SBLIMIT][3][12],
			       frame_params * fr_ps, int *z);

void II_buffer_sample_aug (Bit_stream * bs,
			   frame_params * fr_ps,
			   unsigned int sample[7][3][SBLIMIT],
			   unsigned int bit_alloc[7][SBLIMIT], int gr);

void II_decode_bitalloc_aug (Bit_stream * bs,
			     frame_params * fr_ps,
			     unsigned int bit_alloc[7][SBLIMIT],
			     int *l, int *m, int bits_log);

void mc_aug_composite_status_info (Bit_stream * bs, frame_params * fr_ps,
				   int tca_log, int dynx_log);

void dematricing_aug (double pcm_sample[7][SBLIMIT][3][12],
		      frame_params * fr_ps);

void denormalizing_aug (double pcm_sample[7][SBLIMIT][3][12],
			frame_params * fr_ps);
