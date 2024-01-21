
void II_decode_bitalloc_ml (Bit_stream * bs,
			    frame_params * fr_ps,
			    unsigned int bit_alloc_ml[7][SBLIMIT], int *m);
void II_decode_scale_ml (Bit_stream * bs,
			 frame_params * fr_ps,
			 unsigned int scfsi[7][SBLIMIT],
			 unsigned int bit_alloc[7][SBLIMIT],
			 unsigned int scale_index[7][3][SBLIMIT], int *m);
void II_buffer_sample_ml (Bit_stream * bs,
			  frame_params * fr_ps,
			  unsigned int sample_ml[7][3][SBLIMIT],
			  unsigned int bit_alloc_ml[7][SBLIMIT], int *n_ml_ch);
void II_dequantize_sample_ml (unsigned int sample_ml[7][3][SBLIMIT],
			      unsigned int bit_alloc_ml[7][SBLIMIT],
			      double fraction_ml[7][SBLIMIT][3][12],
			      frame_params * fr_ps, int *n_ml_ch, int *z);
void II_denormalize_sample_ml (double fraction_ml[7][SBLIMIT][3][12],
			       unsigned int scale_index_ml[7][3][SBLIMIT],
			       frame_params * fr_ps,
			       int x, int *n_ml_ch, int *z);
