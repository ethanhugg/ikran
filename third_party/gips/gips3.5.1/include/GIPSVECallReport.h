// GIPSVECallReport.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_CALL_REPORT_H__)
#define __GIPSVE_CALL_REPORT_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

typedef struct {
    int min;					// minumum
	int max;					// maximum
	int average;				// average
} GIPS_stat_val;

typedef struct {				// All levels are reported in dBm0
	GIPS_stat_val speechRx;		// long-term speech levels on receiving side
	GIPS_stat_val speechTx;		// long-term speech levels on transmitting side
	GIPS_stat_val noiseRx;		// long-term noise/silence levels on receiving side
	GIPS_stat_val noiseTx;		// long-term noise/silence levels on transmitting side
} GIPS_P56_statistics;

typedef struct {				// All levels are reported in dB
	GIPS_stat_val ERL;			// Echo Return Loss
	GIPS_stat_val ERLE;			// Echo Return Loss Enhancement
	GIPS_stat_val RERL;			// RERL = ERL + ERLE	
	GIPS_stat_val A_NLP;		// Echo suppression inside EC at the point just before its NLP
} GIPS_echo_statistics;


class VOICEENGINE_DLLEXPORT GIPSVECallReport
{
public:
    static GIPSVECallReport* GetInterface(GIPSVoiceEngine* voiceEngine);

	virtual int GIPSVE_ResetCallReportStatistics(int channel) = 0;

	virtual int GIPSVE_GetSpeechAndNoiseSummary(GIPS_P56_statistics& stats) = 0;
	virtual int GIPSVE_GetEchoMetricSummary(GIPS_echo_statistics& stats) = 0;
	virtual int GIPSVE_GetRoundTripTimeSummary(int channel, GIPS_stat_val& delaysMs) = 0;
	virtual int GIPSVE_GetDeadOrAliveSummary(int channel, int& numOfDeadDetections, int& numOfAliveDetections) = 0;

	virtual int GIPSVE_WriteReportToFile(const char* fileNameUTF8) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVECallReport();
};

#endif    // #if !defined(__GIPSVE_CALL_REPORT_H__)
