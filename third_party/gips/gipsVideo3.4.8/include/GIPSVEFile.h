// GIPSVEFile.h
//
// Copyright (c) 1999-2008 Global IP Solutions. All rights reserved.

#if !defined(__GIPSVE_FILE_H__)
#define __GIPSVE_FILE_H__

#include "GIPSVECommon.h"

class GIPSVoiceEngine;

enum GIPS_FileFormats
{
    FILE_PCM_16KHZ = 0,     // PCM 16 bits/sample, mono, 16 kHz sampling rate
    FILE_WAV = 1,           // PCM or u/A-law, 8/16 bits/sample, mono/stereo, 8,11,16,22,32,44,48 kHz sampling rates
    FILE_COMPRESSED = 2,    // compressed with either iLBC or AMR
    FILE_PCM_8KHZ = 3       // PCM 16 bits/sample, mono, 8 kHz
};

class VOICEENGINE_DLLEXPORT GIPSVEFile
{
public:
    static GIPSVEFile* GetInterface(GIPSVoiceEngine* voiceEngine);

    // Playout file locally 
    virtual int GIPSVE_StartPlayingFileLocally(int channel, const char* fileNameUTF8, bool loop = false, GIPS_FileFormats format = FILE_PCM_16KHZ, float volumeScaling = 1.0,int startPointMs = 0, int stopPointMs = 0) = 0;
    virtual int GIPSVE_StartPlayingFileLocally(int channel, InStream* stream, GIPS_FileFormats format = FILE_PCM_16KHZ, float volumeScaling = 1.0, int startPointMs = 0, int stopPointMs = 0) = 0;
    virtual int GIPSVE_StopPlayingFileLocally(int channel) = 0;
    virtual int GIPSVE_IsPlayingFileLocally(int channel) = 0;
    virtual int GIPSVE_ScaleLocalFilePlayout(int channel, float scale) = 0;

    // Use file as microphone input
    virtual int GIPSVE_StartPlayingFileAsMicrophone(int channel, const char* fileNameUTF8, bool loop = false , bool mixWithMicrophone = false, GIPS_FileFormats format = FILE_PCM_16KHZ, float volumeScaling = 1.0) = 0;
    virtual int GIPSVE_StartPlayingFileAsMicrophone(int channel, InStream* stream, bool mixWithMicrophone = false, GIPS_FileFormats format = FILE_PCM_16KHZ, float volumeScaling = 1.0) = 0;
    virtual int GIPSVE_StopPlayingFileAsMicrophone(int channel) = 0;
    virtual int GIPSVE_IsPlayingFileAsMicrophone(int channel) = 0;
    virtual int GIPSVE_ScaleFileAsMicrophonePlayout(int channel, float scale) = 0;
    
    // Record speaker signal to file
    virtual int GIPSVE_StartRecordingPlayout(int channel, const char* fileNameUTF8, GIPS_CodecInst* compression = NULL, int maxSizeBytes = -1) = 0;
    virtual int GIPSVE_StartRecordingPlayout(int channel, OutStream* stream, GIPS_CodecInst* compression = NULL) = 0;
    virtual int GIPSVE_StopRecordingPlayout(int channel) = 0;
    
    // Record microphone signal to file
    virtual int GIPSVE_StartRecordingMicrophone(const char* fileNameUTF8, GIPS_CodecInst* compression = NULL, int maxSizeBytes = -1) = 0;
    virtual int GIPSVE_StartRecordingMicrophone(OutStream* stream, GIPS_CodecInst* compression = NULL) = 0;
    virtual int GIPSVE_StopRecordingMicrophone() = 0;
    
    // Record mixed (speaker + microphone) signal to file
    virtual int GIPSVE_StartRecordingCall(const char* fileNameUTF8, GIPS_CodecInst* compression = NULL, int maxSizeBytes = -1) = 0;
    virtual int GIPSVE_StartRecordingCall(OutStream* stream, GIPS_CodecInst* compression = NULL) = 0;
    virtual int GIPSVE_PauseRecordingCall(bool enable) = 0;    
    virtual int GIPSVE_StopRecordingCall() = 0;

    // Record left/right speaker signals to file
    virtual int GIPSVE_StartRecordingPlayoutStereo(const char* fileNameLeftUTF8, const char* fileNameRightUTF8, GIPS_StereoChannel select, GIPS_CodecInst* compression = NULL) = 0;
    virtual int GIPSVE_StartRecordingPlayoutStereo(OutStream* streamLeft, OutStream* streamRight, GIPS_StereoChannel select, GIPS_CodecInst* compression = NULL) = 0;
    virtual int GIPSVE_StopRecordingPlayoutStereo(GIPS_StereoChannel select) = 0;

    // Conversion between different file formats
    virtual int GIPSVE_ConvertPCMToWAV(const char* fileNameInUTF8, const char* fileNameOutUTF8) = 0;
    virtual int GIPSVE_ConvertPCMToWAV(InStream* streamIn, OutStream* streamOut, int lengthInBytes) = 0;
    virtual int GIPSVE_ConvertWAVToPCM(const char* fileNameInUTF8, const char* fileNameOutUTF8) = 0;
    virtual int GIPSVE_ConvertWAVToPCM(InStream* streamIn, OutStream* streamOut) = 0;
    virtual int GIPSVE_ConvertPCMToCompressed(const char* fileNameInUTF8, const char* fileNameOutUTF8, GIPS_CodecInst* compression) = 0;
    virtual int GIPSVE_ConvertPCMToCompressed(InStream* streamIn, OutStream* streamOut, GIPS_CodecInst* compression) = 0;
    virtual int GIPSVE_ConvertCompressedToPCM(const char* fileNameInUTF8, const char* fileNameOutUTF8) = 0;
    virtual int GIPSVE_ConvertCompressedToPCM(InStream* streamIn, OutStream* streamOut) = 0;

    // Conversion from RTP packets to file
    virtual int GIPSVE_InitRTPToFileConversion(const char* fileNameUTF8, unsigned int conversionDelay, GIPS_CodecInst* compression = NULL) = 0;
    virtual int GIPSVE_InitRTPToFileConversion(OutStream* stream, unsigned int conversionDelay, GIPS_CodecInst* compression = NULL) = 0;
    virtual int GIPSVE_StartRTPToFileConversion(int channel) = 0;        
    virtual int GIPSVE_StopRTPToFileConversion(int channel) = 0;    
    virtual int GIPSVE_ConvertRTPToFile(int channel, char* rtpPacketBuffer, unsigned int length, unsigned int incomingTimeStamp) = 0; 
    
    // Misc file functions
    virtual int GIPSVE_GetFileDuration(const char* fileNameUTF8, int& durationMs, GIPS_FileFormats format = FILE_PCM_16KHZ) = 0;
    virtual int GIPSVE_GetPlaybackPosition(int channel, int& positionMs) = 0;

    virtual int Release() = 0;
protected:
    virtual ~GIPSVEFile();
};

#endif    // #if !defined(__GIPSVE_FILE_H__)
