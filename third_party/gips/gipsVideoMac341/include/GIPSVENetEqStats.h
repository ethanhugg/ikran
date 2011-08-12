// GIPSVENetEqStats.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_NETEQ_STATS_H__)
#define __GIPSVE_NETEQ_STATS_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

typedef struct {
    unsigned short currentBufferSize;     /* current jitter buffer size in ms */
    unsigned short preferredBufferSize;   /* preferred (optimal) buffer size in ms */
    unsigned short currentPacketLossRate; /* loss rate (network + late) in percent (in Q14) */
    unsigned short currentDiscardRate;    /* late loss rate in percent (in Q14) */
    unsigned short currentExpandRate;     /* fraction (of original stream) of synthesized speech inserted through expansion (in Q14) */
    unsigned short currentPreemptiveRate; /* fraction of synthesized speech inserted through pre-emptive expansion (in Q14) */
    unsigned short currentAccelerateRate; /* fraction of data removed through acceleration (in Q14) */
} GIPS_NETEQ_networkStatistics;

typedef struct {
    unsigned int jbMinSize;                 /* smallest Jitter Buffer size during call in ms */
    unsigned int jbMaxSize;                 /* largest Jitter Buffer size during call in ms */
    unsigned int jbAvgSize;                 /* the average JB size, measured over time - ms */
    unsigned int jbChangeCount;             /* number of times the Jitter Buffer changed (using Accelerate or Pre-emptive Expand) */
    unsigned int lateLossMs;                /* amount (in ms) of audio data received late */
    unsigned int accelerateMs;              /* milliseconds removed to reduce jitter buffer size */
    unsigned int flushedMs;                 /* milliseconds discarded through buffer flushing */
    unsigned int generatedSilentMs;         /* milliseconds of generated silence */
    unsigned int interpolatedVoiceMs;       /* milliseconds of synthetic audio data (non-background noise) */
    unsigned int interpolatedSilentMs;      /* milliseconds of synthetic audio data (background noise level) */
    unsigned int countExpandMoreThan120ms;  /* count of tiny expansions in output audio */
    unsigned int countExpandMoreThan250ms;  /* count of small expansions in output audio */
    unsigned int countExpandMoreThan500ms;  /* count of medium expansions in output audio */
    unsigned int countExpandMoreThan2000ms; /* count of long expansions in output audio */
    unsigned int longestExpandDurationMs;   /* duration of longest audio drop-out */
    unsigned int countIAT500ms;             /* count of times we got small network outage (inter-arrival time in [500, 1000) ms) */
    unsigned int countIAT1000ms;            /* count of times we got medium network outage (inter-arrival time in [1000, 2000) ms) */
    unsigned int countIAT2000ms;            /* count of times we got large network outage (inter-arrival time >= 2000 ms) */
    unsigned int longestIATms;              /* longest packet inter-arrival time in ms */
    unsigned int minPacketDelayMs;          /* min time incoming Packet "waited" to be played */
    unsigned int maxPacketDelayMs;          /* max time incoming Packet "waited" to be played */
    unsigned int avgPacketDelayMs;          /* avg time incoming Packet "waited" to be played */
} GIPS_NETEQ_jitterStatistics;


class VOICEENGINE_DLLEXPORT GIPSVENetEqStats
{
public:
	static GIPSVENetEqStats* GetInterface(GIPSVoiceEngine* voiceEngine);

	// Get the "in-call" statistics from NetEQ.
	// The statistics are reset after the query.
	virtual int GIPSVE_GetNetworkStatistics(int channel, GIPS_NETEQ_networkStatistics& stats) = 0;
	
	// Get the "post-call" jitter statistics from NetEQ.
	// The statistics are not reset by the query. Use the function NETEQ_GIPS_resetJitterStatistics
	// to reset the statistics.
	virtual int GIPSVE_GetJitterStatistics(int channel, GIPS_NETEQ_jitterStatistics& stats) = 0;
	
	// Get the optimal buffer size calculated for the current network conditions.
	virtual int GIPSVE_GetPreferredBufferSize(int channel, unsigned short& preferredBufferSize) = 0;
	
	// Reset "post-call" jitter statistics.
	virtual int GIPSVE_ResetJitterStatistics(int channel) = 0;

	virtual int Release() = 0;

protected:
	virtual ~GIPSVENetEqStats();
};

#endif	// #if !defined(__GIPSVE_NETEQ_STATS_H__)
