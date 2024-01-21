
void buffer_CRC (Bit_stream * bs, unsigned int *old_crc);
void recover_CRC_error (long pcm_sample[7][3][SBLIMIT],
			int error_count,
			frame_params * fr_ps,
			FILE * outFile, unsigned long *psampFrames, int ch);
