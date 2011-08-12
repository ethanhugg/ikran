/*
 * GIPSVideoCodecTypes.h
 *
 * This file contains definitions for GIPS video codec types
 *
 * Copyright (c) 2003-2009 by Global IP Solutions.
 * All rights reserved.
 * 
 */

#ifndef GIPS_VIDEO_CODEC_TYPES_H
#define GIPS_VIDEO_CODEC_TYPES_H

enum GIPSVideoProfile
{
    GIPS_PROFILE_BASE  = 0x00,
    GIPS_PROFILE_MAIN  = 0x01
};

enum GIPSVideoComplexity
{
    GIPS_COMPLEXITY_NORMAL = 0x00,
    GIPS_COMPLEXITY_HIGH   = 0x01,
    GIPS_COMPLEXITY_HIGHER = 0x02,
    GIPS_COMPLEXITY_MAX    = 0x03
};

#define GIPS_CONFIG_PAPRAMETER_SIZE 128
#define GIPS_PAYLOAD_NAME_SIZE 32
#define GIPS_DEPENDENCY_SIZE 8
#define GIPS_SVC_MAXLAYERS 10

enum GIPSVideo_LayerTypes
{
	GIPS_EXTEND_2X2 = 0,  // spatial extend twice in both direction (spatial scalability)
	GIPS_EXTEND_1X1 = 1,  // no spatial extend (coarse-grain quality scalability)
	GIPS_EXTEND_MGS = 2,  // no spatial extend (medium-grain quality scalability)
	GIPS_EXTEND_1_5 = 15, // spatial 1.5 extend in both direction
	GIPS_EXTEND_CUSTOM = 100 // custom spatial scalability
};

struct GIPSVideo_LayerRange
{
    unsigned char DIDlow;   // dependency low 
    unsigned char QIDlow;   // quality low
    unsigned char TIDlow;   // temporal low
    unsigned char DIDhigh;  // dependency high
    unsigned char QIDhigh;  // quality high
    unsigned char TIDhigh;  // temporal high
};

struct GIPSVideo_LayerProperties
{
    float cumulativeBitRate;
    GIPSVideo_LayerTypes layerType;
	int frame_width;                // must be specified for GIPS_EXTEND_CUSTOM layers
	int frame_height;               // must be specified for GIPS_EXTEND_CUSTOM layers
};

struct GIPSVideo_Layers
{
    int numOfLayers;
    int temporalScaleFactor;
    GIPSVideo_LayerProperties layer[GIPS_SVC_MAXLAYERS];
};

struct GIPSVideo_CodecInst
{
	unsigned char pltype;
	char plname[GIPS_PAYLOAD_NAME_SIZE];
	int bitRate;				// start bitrate Kbit/s
	int maxBitRate;				// max bitrate Kbit/s
	unsigned char frameRate;	// Max frames per sec
	unsigned short height;
	unsigned short width;
	char quality;	
	char level;		
	char codecSpecific;			
	unsigned char configParameterSize; // set to non 0 when calling set sendcodec to enable outband signaling
	unsigned char configParameters[GIPS_CONFIG_PAPRAMETER_SIZE];
	int minBitRate;				// min bitrate Kbit/s
    char profile;
    char complexity;
    unsigned char dependency[GIPS_DEPENDENCY_SIZE];
    GIPSVideo_LayerRange layerRange;
    GIPSVideo_Layers layers;
};

#endif
