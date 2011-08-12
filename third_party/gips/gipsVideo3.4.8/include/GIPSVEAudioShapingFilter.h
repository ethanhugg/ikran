// GIPSVEAudioShapingFilter.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_AUDIO_SHAPING_FILTER_H__)
#define __GIPSVE_AUDIO_SHAPING_FILTER_H__

#include "GIPSVEExternalMedia.h"  // GIPSVEMediaProcess

/* ----------------------------------------------------------------------------
    GIPSVEAudioShapingFilter

    This class is an implementation of the abstract GIPSVEMediaProcess class.
    It implements a shaping (high-pass filter) with a stop-band suppression of 
    approx. -40 dB and has an adjustable roll-off frequency (approx. -1dB)
    between 80 and 240 Hz. The roll-off frequency is set at construction.
  
    Example:
    ========

        - the high-pass filter is installed on the recording side in VE;
        - filter roll-off is 240 Hz (type=9);
        - only channel 0 is affected by the filter;
        - error handling is omitted;
        - usage of GIPSVEBase object is omitted in the example;

    // FilterUser.h

    #include "GIPSVEBase.h" 
    #include "GIPSVEExternalMedia.h"
    #include "GIPSVEAudioShapingFilter.h"

    public:
        void Create();
        void ApplyFilter();
        void Delete();

    private:
        GIPSVoiceEngine* ve; 
        GIPSVEBase* base;
        GIPSVEExternalMedia* xmedia;
        GIPSVEAudioShapingFilter* filter;

    // FilterUser.cpp

    void FilterUser::Create()
    {
        ve = GIPSVoiceEngine::Create();
        base = GIPSVEBase::GIPSVE_GetInterface(ve);
        xmedia = GIPSVEExternalMedia::GetInterface(ve);
        filter = GIPSVEAudioShapingFilter::Create(9);  // 240 Hz roll-off
    }

    void FilterUser::ApplyFilter()
    {
        // Filter mic signal on channel 0 using the GIPSVEMediaProcess derived audio-shaping filter.
        xmedia->GIPSVE_SetExternalMediaProcessing(RECORDING_PER_CHANNEL, 0, true, *filter);
    }

    void FilterUser::Delete()
    {
        base->GIPSVE_Release();
        xmedia->Release();
        GIPSVoiceEngine::Delete(ve);
        delete filter;
    }

  ---------------------------------------------------------------------------- */

class GIPSVEAudioShapingFilter : public GIPSVEMediaProcess
{
public:	
    // This function creates the shaping (high-pass) filter. The filter has a 
    // stop-band suppression of approx. -40 dB and has an adjustable roll-off
    // frequency (approx. -1dB). The input parameter type sets the roll-off
    // frequency between approx. 80 Hz (type=0) and 240 Hz (type=9).
    //
	static GIPSVEAudioShapingFilter* Create(int type);

    // This function applies a shaping filter to an input signal vector.
    // The filtering is performed at 8 or 16kHz depending on the encoder.
    //
    //void Process(int channelNumber, short* audio10ms16kHz, int length, int samplingFreq);
	void Process(int channelNumber, short* audioLeft10ms, short* audioRight10ms, int length, int samplingFreq, bool isStereo);
public:
	~GIPSVEAudioShapingFilter();

private:	// two-phase construction
	GIPSVEAudioShapingFilter(int type);
	int Construct();

private:	// declarations only to prevent copying
	GIPSVEAudioShapingFilter(const GIPSVEAudioShapingFilter&);
	GIPSVEAudioShapingFilter& operator=(const GIPSVEAudioShapingFilter&);

private:
	void*	_filter;
	int		_samplingFreqHz;	// sampling frequency: 8000 or 16000 Hz
	int		_type;				// roll-off frequency: 0-9 <=> 80-240 Hz 
	bool	_isStereo;
};

#endif    // #if !defined(__GIPSVE_AUDIO_SHAPING_FILTER_H__)
