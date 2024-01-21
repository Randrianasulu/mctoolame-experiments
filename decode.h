
void decode_info (Bit_stream * bs, frame_params * fr_ps);
void II_decode_bitalloc (Bit_stream * bs,
			 frame_params * fr_ps,
			 unsigned int bit_alloc[7][SBLIMIT], int bits_log);
void II_decode_scale (Bit_stream * bs,
		      frame_params * fr_ps,
		      unsigned int scfsi[7][SBLIMIT],
		      unsigned int bit_alloc[7][SBLIMIT],
		      unsigned int scale_index[7][3][SBLIMIT],
		      int *l, int *m, int scfsi_log);
void II_buffer_sample (Bit_stream * bs,
		       frame_params * fr_ps,
		       unsigned int sample[7][3][SBLIMIT],
		       unsigned int bit_alloc[7][SBLIMIT]);
void II_dequantize_sample (unsigned int sample[7][3][SBLIMIT],
			   unsigned int bit_alloc[7][SBLIMIT],
			   double fraction[7][SBLIMIT][3][12],
			   frame_params * fr_ps, int *z);
void II_lfe_calc (frame_params * fr_ps);
void II_denormalize_sample (double fraction[7][SBLIMIT][3][12],
			    unsigned int scale_index[7][3][SBLIMIT],
			    frame_params * fr_ps, int x, int *z);
