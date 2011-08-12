/*
 *
 *  GipsVideoEngineMacCarbonh
 *  Created by: Magnus Flodman
 *  Date      : 2005-10-01
 *
 *  Public API for GIPS VideoEngine on a Mac platforms and Carbon window handling system.
 * 
 *  Copyright (c) 2005
 *  Global IP Sound AB, Organization number: 5565739017
 *  landsgatan 42, SE-116 63 Stockholm, Sweden
 *  All rights reserved.
 *
 *  Note: Gips VideoEngineMacCarbon has the Macintosh specific implementation
 *  
 *
 */
#ifndef PUBLIC_GIPS_VIDEO_ENGINE_MAC_CARBON_H
#define PUBLIC_GIPS_VIDEO_ENGINE_MAC_CARBON_H

#include "GipsVideoEngine.h"


class VIDEOENGINE_DLLEXPORT GipsVideoEngineMacCarbon : public GipsVideoEngine
{
public:
    virtual int GIPSVideo_AddLocalRenderer(WindowRef localWindow, int zOrder, float left, float top, float right, float bottom) = 0;
    virtual int GIPSVideo_AddLocalRenderer(HIViewRef localWindow, int zOrder, float left, float top, float right, float bottom) = 0;
    virtual int GIPSVideo_AddRemoteRenderer(int channel, WindowRef remoteWindow, int zOrder, float left, float top, float right, float bottom) = 0;
    virtual int GIPSVideo_AddRemoteRenderer(int channel, HIViewRef remoteWindow, int zOrder, float left, float top, float right, float bottom) = 0;
	virtual int GIPSVideo_AddRemoteDemuxRenderer(int channel, int streamId, WindowRef renderWindow, int zOrder, float left, float top, float right, float bottom) = 0;	
    virtual int GIPSVideo_ConfigureMixer(int channel, int streamID, WindowRef renderWindow, float left, float top, float right, float bottom) = 0;

    virtual int GIPSVideo_EnableOpenGLRendering(bool enable) = 0;

    virtual void GIPSVideo_RenderLock() = 0;
    virtual void GIPSVideo_RenderUnLock() = 0;
	

    virtual ~GipsVideoEngineMacCarbon();
};
VIDEOENGINE_DLLEXPORT GipsVideoEngineMacCarbon &GetGipsVideoEngine();
VIDEOENGINE_DLLEXPORT GipsVideoEngineMacCarbon* GetNewVideoEngine();
#endif
