
#define VERSION "$Revision: 2.2 $"

/***********************************************************************
*
*  Global Include Files
*
***********************************************************************/
#include <stdint.h>
#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include        <unistd.h>	 /* removed 92-08-05 shn */
#include	<stdlib.h>       /* put in 92-08-05 shn */

/***********************************************************************
*
*  Global Definitions
*
***********************************************************************/
extern int pred_coef_table[6][16];  /* def. in decode.c */
extern double S_freq;
extern int Bitrate, Frame_Bits;
/* General Definitions */

#define         FLOAT                   float

#define         FALSE                   0
#define         TRUE                    (!FALSE)
#define         NULL_CHAR               '\0'

#define         MAX_U_32_NUM            0xFFFFFFFF
#define         PI                      3.14159265358979
#define         PI4                     PI/4
#define         PI64                    PI/64
#define         LN_TO_LOG10             0.2302585093

#define         VOL_REF_NUM             0
#define         MPEG_AUDIO_ID           1
#define         MAC_WINDOW_SIZE         24

#define         MONO                    1
#define         STEREO                  2
#define         BITS_IN_A_BYTE          8
#define         WORD                    16
#define         MAX_NAME_SIZE           81
#define         SBLIMIT                 32
#define         FFT_SIZE                1024
#define         HAN_SIZE                512
#define         SCALE_BLOCK             12
#define         SCALE_RANGE             64
#define         SCALE                   32768.0
#define         CRC16_POLYNOMIAL        0x8005

/* Sync - Word for multichannel extern bitstream */
#define		EXT_SYNCWORD	        0x7ff

/* MPEG Header Definitions - Mode Values */

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define		MPG_MD_MONO		3
#define		MPG_MD_NONE		4

/* Multi-channel Definitions - Mode Values */

#define		MPG_MC_STEREO 	       0
#define		MPG_MC_NONE	       6

/* AIFF Definitions */

#ifndef	MS_DOS
#define         IFF_ID_FORM             "FORM" /* HP400 unix v8.0: double qoutes 1992-07-24 shn */
#define         IFF_ID_AIFF             "AIFF"
#define         IFF_ID_COMM             "COMM"
#define         IFF_ID_SSND             "SSND"
#define         IFF_ID_MPEG             "MPEG"
#else
#define         IFF_ID_FORM             "FORM"
#define         IFF_ID_AIFF             "AIFF"
#define         IFF_ID_COMM             "COMM"
#define         IFF_ID_SSND             "SSND"
#define         IFF_ID_MPEG             "MPEG"
#endif

/* "bit_stream.h" Definitions */

#define         MINIMUM         4    /* Minimum size of the buffer in bytes */
#define         MAX_LENGTH      32   /* Maximum length of word written or
						read from bit stream */
#define         READ_MODE       0
#define         WRITE_MODE      1
#define         ALIGNING        8
#define         BINARY          0
#define         ASCII           1
#define         BS_FORMAT       ASCII /* BINARY or ASCII = 2x bytes */
#define         BUFFER_SIZE     4096

#define         MIN(A, B)       ((A) < (B) ? (A) : (B))
#define         MAX(A, B)       ((A) > (B) ? (A) : (B))

/***********************************************************************
*
*  Global Type Definitions
*
***********************************************************************/

/* Structure for Reading Layer II Allocation Tables from File */

typedef struct {
		unsigned int    steps;
		unsigned int    bits;
		unsigned int    group;
		unsigned int    quant;
} sb_alloc, *alloc_ptr;

typedef sb_alloc        al_table[SBLIMIT][16];

/* Header Information Structure */

typedef struct {
	int version;
	int lay;
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int center;		   /* center present */
	int surround; 		   /* surrounds present*/
	int dematrix_procedure;	   /* type of dematricing */
	int lfe;		   /* low freequency effect channel*/

	unsigned int	lfe_alloc;
	unsigned int	lfe_scf;
	unsigned int	lfe_spl[12];
	double		lfe_spl_fraction[12]; 

	int audio_mix;		   /* large or small room */
	int no_of_multi_lingual_ch;/* number of multilingual channels */
	int multi_lingual_fs;	   /* fs of main channels and ML-fs are the same*/
	int multi_lingual_layer;   /* Layer IIML or Layer II ML is used*/
	int mc_prediction[8];	   /* prediction bit *//* 28.9. R.S. changed name and length like DIS*/
	int mc_pred_coeff[8][6][3];/* bits for prediction coefficient */
	int tc_alloc[12];	   /* transmission channel allocation in each sbgroup*/
	int mc_predsi[8][6];	   /* predictor select information*/
	int mc_delay_comp[8][6];   /* 3 bit for delay compensations factor */
	int ext_bit_stream_present;
	int copyright_ident_bit;   /* additional copyright bits */
	int copyright_ident_start; /*                           */
	int n_ad_bytes;	
	int mc_prediction_on;	   
	int tc_sbgr_select;
	int dyn_cross_on;
	int dyn_cross_LR;
	int dyn_cross_mode[12];
	int dyn_second_stereo[12]; /* 960816 FdB changed from scalar to array */
	int tc_allocation;
	int ext_syncword;         /* 12 bits */
	int ext_crc_check;        /* 16 bits */
	int ext_length;           /* 11 bits *//* Important!! in bits per frame */
	int reserved_bit;
unsigned int ext_crc_bits[15];
#ifdef Augmentation_7ch
	int aug_mtx_proc;
	int aug_dyn_cross_on;
	int aug_future_ext;
	int tc_aug_alloc[12];	   /* transmission channel allocation in each sbgroup */
	int dyn_cross_aug_mode[12];
#endif
} layer, *the_layer;

/* "bit_stream.h" Type Definitions */

typedef struct  bit_stream {
    FILE        *pt;            /* pointer to bit stream device */
    unsigned char *bits;        /* bit stream bit buffer */
    int         header_size;	/* header of bitstream (in number of bytes) */
    int32_t        totbits;        /* bit counter of bit stream */
    int32_t        curpos;         /* bit pointer of bit stream */
    int         mode;           /* bit stream open in read or write mode */
    int         eobs;           /* end of bit stream flag */
    char        format;		/* format of file in rd mode (BINARY/ASCII) */
} Bit_stream;

/* Parent Structure Interpreting some Frame Parameters in Header */

typedef struct {
	layer		*header;	/* raw header information */
	Bit_stream	*bs_mpg;
	Bit_stream	*bs_ext;
	Bit_stream	*bs_mc;
	int		actual_mode;    /* when writing IS, may forget if 0 chs */
	al_table	*alloc;         /* bit allocation table read in */
	al_table	*alloc_mc;      /* MC bit allocation table read in */
	al_table	*alloc_ml;      /* ML bit allocation table read in */
	int		tab_num; 	/* number of table as loaded */
	int		tab_num_mc; 	/* number of MC-table as loaded */
	int		tab_num_ml;     /* number of ML-table as loaded */
	int		stereo;         /* 1 for mono, 2 for stereo */
	int		mc_channel;
	int		jsbound; 	/* first band of joint stereo coding */
	double		mnr_min;	/* mnr for dynamic bitallocation */
	int		sblimit;	/* total number of sub bands */
	int		sblimit_mc;     /* total number of MC sub bands */
	int		sblimit_ml;     /* total number of ML sub bands */
	int		alloc_bits;	/* to read the right length of tc_alloc field */  
	int		dyn_cross_bits; /* to read the right length of dyn_cross field */
	int		pred_mode;      /* entry for prediction table */
} frame_params;

/* Double and SANE Floating Point Type Definitions */

typedef struct  IEEE_DBL_struct {
				uint32_t   hi;
				uint32_t   lo;
} IEEE_DBL;

typedef struct  SANE_EXT_struct {
				uint32_t   l1;
				uint32_t   l2;
				uint16_t  s1;
} SANE_EXT;

/* AIFF Type Definitions */

typedef char	 ID[4];

typedef struct  identifier_struct{
		 ID name;
		 int32_t ck_length;
}identifier;


typedef struct  ChunkHeader_struct {
				ID      ckID;
				int32_t    ckSize;
} ChunkHeader;

typedef struct  Chunk_struct {
				ID      ckID;
				int32_t    ckSize;
				ID      formType;
} Chunk;

typedef struct  CommonChunk_struct {
				ID              ckID;
				int32_t            ckSize;
				int16_t           numChannels;
				uint32_t   numSampleFrames;
				int16_t           sampleSize;
				uint8_t            sampleRate[10];
} CommonChunk;

typedef struct  SoundDataChunk_struct {
				ID              ckID;
				int32_t            ckSize;
				uint32_t   offset;
				uint32_t   blockSize;
} SoundDataChunk;

typedef struct  blockAlign_struct {
				uint32_t   offset;
				uint32_t   blockSize;
} blockAlign;

typedef struct  IFF_AIFF_struct {
				int16_t           numChannels;
                uint32_t   numSampleFrames;
                int16_t           sampleSize;
		double          sampleRate;
                ID/*char**/     sampleType;/*must be allocated 21.6.93 SR*/
                blockAlign      blkAlgn;
} IFF_AIFF;

/***********************************************************************
*
*  Global Variable External Declarations
*
***********************************************************************/
extern layer    info;
extern char     *mode_names[4];
extern char     *layer_names[3];
extern double   s_freq[4];
extern int	bitrate[3][15];
extern double	multiple[64];
extern int      sb_groups[12];
/***********************************************************************
*
*  Global Function Prototype Declarations
*
***********************************************************************/

/* The following functions are in the file "common.c" */

extern int            js_bound(int, int);
extern void           hdr_to_frps(frame_params*);
extern void	      mc_hdr_to_frps(frame_params*);
extern void           WriteHdr(frame_params*, FILE*);
extern void           *mem_alloc(uint32_t, char*);
extern void           mem_free(void**);
extern void           double_to_extended(double*, uint8_t[10]);
extern void           extended_to_double(uint8_t[10], double*);
extern int	      aiff_read_headers(FILE*, IFF_AIFF*, int*);
extern int	      aiff_seek_to_sound_data(FILE*);
extern int            aiff_write_headers(FILE*, IFF_AIFF*);
extern int            open_bit_stream_r(Bit_stream*, char*, int);
extern void           close_bit_stream_r(Bit_stream*);
extern unsigned int   get1bit(Bit_stream*);
extern uint32_t  getbits(Bit_stream*, int);
extern void	      program_information(void);
extern int            end_bs(Bit_stream*);
extern int            seek_sync_mpg(Bit_stream*);
extern int            seek_sync_ext(Bit_stream*, frame_params*);
extern void           update_CRC(unsigned int, unsigned int, unsigned int*);
extern void           II_CRC_calc(frame_params*, unsigned int[7][SBLIMIT],
						unsigned int[7][SBLIMIT], unsigned int*);
extern void           mc_error_check(frame_params*, unsigned int[7][SBLIMIT],
						unsigned int[7][SBLIMIT], unsigned int*, int, int);
#ifdef Augmentation_7ch
extern void           mc_aug_error_check(frame_params*, unsigned int[7][SBLIMIT],
						unsigned int[7][SBLIMIT], unsigned int*);
#endif
extern int	      mc_ext_error_check(frame_params*, int, int);

