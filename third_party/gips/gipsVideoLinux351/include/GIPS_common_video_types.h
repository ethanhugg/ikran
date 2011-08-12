#ifndef GIPS_COMMON_VIDEO_TYPES_H
#define GIPS_COMMON_VIDEO_TYPES_H

	enum GIPSVideoSize
	{
		GIPS_UNDEFINED, 
		GIPS_SQCIF,     // 128*96       = 12 288
		GIPS_QQVGA,     // 160*120      = 19 200
		GIPS_QCIF,      // 176*144      = 25 344
        GIPS_CGA,       // 320*200      = 64 000
		GIPS_QVGA,      // 320*240      = 76 800
        GIPS_SIF,       // 352*240      = 84 480
		GIPS_WQVGA,     // 400*240      = 96 000
		GIPS_CIF,       // 352*288      = 101 376
        GIPS_W288P,     // 512*288      = 147 456 (WCIF)
        GIPS_448P,      // 576*448      = 281 088
		GIPS_VGA,       // 640*480      = 307 200
        GIPS_432P,      // 720*432      = 311 040
        GIPS_W432P,     // 768*432      = 331 776
        GIPS_4SIF,      // 704*480      = 337 920
        GIPS_W448P,     // 768*448      = 344 064
		GIPS_NTSC,		// 720*480      = 345 600
        GIPS_FW448P,    // 800*448      = 358 400
		GIPS_WVGA,      // 800*480      = 384 000
		GIPS_4CIF,      // 704×576      = 405 504
		GIPS_SVGA,      // 800*600      = 480 000
        GIPS_W544P,     // 960*544      = 522 240
        GIPS_W576P,     // 1024*576     = 589 824 (W4CIF)
		GIPS_HD,        // 960*720      = 691 200
		GIPS_XGA,       // 1024*768     = 786 432
		GIPS_WHD,       // 1280*720     = 921 600
		GIPS_FULL_HD,   // 1440*1080    = 1 555 200
		GIPS_WFULL_HD,  // 1920*1080    = 2 073 600

		GIPS_NUMBER_OF_VIDEO_SIZE
	};

	enum GIPSVideoType
	{
		GIPS_UNKNOWN,
		GIPS_I420,
		GIPS_IYUV,
		GIPS_RGB24,
		GIPS_ARGB,
		GIPS_ARGB4444,
		GIPS_RGB565,
		GIPS_ARGB1555,
		GIPS_YUY2,
		GIPS_YV12,
		GIPS_UYVY,
		GIPS_V210,
		GIPS_HDYC,
        GIPS_MJPG,
        GIPS_H263,
        GIPS_H264,

        GIPS_NUMBER_OF_VIDEO_TYPES
	};

	enum GIPSVideo_FrameTypes
	{
		GIPS_KEY_FRAME,
		GIPS_DELTA_FRAME,
		GIPS_GOLDEN_FRAME, 
		GIPS_KEY_DELTA_FRAME 
	};

	enum GIPSVideoQuality
	{
		GIPS_QUALITY_DEFAULT,
		GIPS_QUALITY_MIN_FRAME_RATE,
		GIPS_QUALITY_VIDEO,
		GIPS_QUALITY_TALKING_HEAD
	};

	enum GIPSH264Mode 
	{
		GIPS_H264SingleMode,
		GIPS_H264NonInterLeavedMode // allows both STAP-A and FU-A mode
    };

	enum GIPSH264SVCMode 
	{
		GIPS_H264SVC_Single     = 0,
		GIPS_H264SVC_FU_A       = 1,
		GIPS_H264SVC_STAP_A     = 3,    // allows both STAP-A and FU-A mode
        GIPS_H264SVC_NI_C       = 4,    // Decode order 
        GIPS_H264SVC_NI_T_Mode  = 8,    // TimeStamp order
        GIPS_H264SVC_NI_TC_Mode = 12    // Decode order and TimeStamp order
    };

	enum GIPSVideoSignalingLSVX
	{
		GIPS_LSVX_KEY     = 0x00,
		GIPS_LSVX_NACK    = 0x01,
		GIPS_LSVX_FEC     = 0x02,
		GIPS_LSVX_RECEIVE = 0x04
	};

	enum GIPSH263FrameDrop
	{
		GIPS_H263_DECODE_P_FRAMES,
		GIPS_H263_DROP_P_FRAMES
	};
	
	enum GIPSFlashMode
	{
		GIPS_FLASH_NONE			   = 0x0000,
		GIPS_FLASH_AUTO			   = 0x0001,
		GIPS_FLASH_FORCED		   = 0x0002,
		GIPS_FLASH_FILL_IN		   = 0x0004,
		GIPS_FLASH_RED_EYE_REDUCE  = 0x0008,
		GIPS_FLASH_SLOW_FRONT_SYNC = 0x0010,
		GIPS_FLASH_SLOW_REAR_SYNC  = 0x0020, 
		GIPS_FLASH_MANUAL		   = 0x0040 
	};

    enum GIPSScreenOrientation
	{
		GIPS_ORIENTATION_PORTRAIT,
		GIPS_ORIENTATION_LANDSCAPE
	};

    enum GIPSVideoLayouts
    {
        GIPS_LAYOUT_NONE,
        GIPS_LAYOUT_DEFAULT,
        GIPS_LAYOUT_ADVANCED1,
        GIPS_LAYOUT_ADVANCED2,
        GIPS_LAYOUT_ADVANCED3,
        GIPS_LAYOUT_ADVANCED4,
        GIPS_LAYOUT_FULL
    };

	struct GIPSVideo_CallStatistics
	{
		unsigned short fraction_lost;
		unsigned long cum_lost;
		unsigned long ext_max;
		unsigned long jitter;
		int RTT;
		int bytesSent;
		int packetsSent;
		int bytesReceived;
		int packetsReceived;
	};

	struct GIPSVideo_FrameStatistics
	{
		unsigned int sentKeyFrames;
		unsigned int sentDeltaFrames;

		unsigned int receivedKeyFrames;
		unsigned int receivedDeltaFrames;
	    
		unsigned int zeroEncodeFrames;

		unsigned int zeroDecodeFrames;
		unsigned int errorDecodeFrames;
	};

#endif
