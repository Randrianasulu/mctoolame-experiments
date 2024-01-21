

#define  DFLT_IPEXT_MPG	   ".mp2" 	/* default input for base file */
#define  DFLT_IPEXT_EXT	   ".ext" 	/* default input for extension file */
#define  DFLT_OPEXT_DEC	   ".aiff"  	/* default output file name extension */
#define  DFLT_OPEXT_LFE	   "_lfe.dec" 	/* 10/31/95 R.S. output for lfe pcm-file */
#define  DFLT_OPEXT_ML	   ".ml" 	/* default MultiLingual output file name extension */
#define	 FILTYP_DEC_AIFF   "AIFF"	/* '-> " . 7/13/92. sr */
#define	 FILTYP_DEC_BNRY   "TEXT"	/* '-> " . 7/13/92. sr */
#define	 CREATR_DEC_AIFF   "Sd2a"	/* '-> " . 7/13/92. sr */
#define	 CREATR_DEC_BNRY   "????"	/* '-> " . 7/13/92. sr */

#define   SYNC_WORD         (int32_t) 0xfff
#define   SYNC_WORD_LNGTH   12

#define   MUTE              0
#define   STEP		    0.03125	 /* 13.10.93 R.S. step for prediction */
#define   PREDDEL           (2+7)        /* max. delay in prediction, HP 08-nov-94 */

#define   EXTENSION_CRC_CHECK

/* The following functions are in the file "musicout.c" */




#ifdef Augmentation_7ch
extern void   II_buffer_sample_aug(Bit_stream*, frame_params*, unsigned int[7][3][SBLIMIT],
					   unsigned int[7][SBLIMIT], int);
extern void   II_dequantize_sample_aug(unsigned int[7][3][SBLIMIT],
					   unsigned int[7][SBLIMIT], double[7][SBLIMIT][3][12],
					   frame_params*, int*);
extern void   II_denormalize_sample_aug(double[7][SBLIMIT][3][12],
					   unsigned int[7][3][SBLIMIT], frame_params*, int, int*);
extern void   II_decode_bitalloc_aug(Bit_stream*, frame_params*, unsigned int[7][SBLIMIT], int*, int*, int);
extern void   mc_aug_composite_status_info(Bit_stream*, frame_params*, int, int);
extern void   denormalizing_aug(double[7][SBLIMIT][3][12], frame_params*); 
extern void   dematricing_aug(double[7][SBLIMIT][3][12], frame_params*);
#endif
