/*
 *
 *	GipsVideoEngineLinux.h
 *	Created by: Tomas Lundqvist
 *	Date      : 2007-09-17
 *
 *	Public API for GIPS VideoEngine on a Linux platform.
 * 
 *	Copyright (c) 2007
 *	Global IP Solutions AB, Organization number: 5565739017
 *	Magnus Ladulasgatan 63B, SE-118 27 Stockholm, Sweden
 *	All rights reserved.
 *
 *	Note: Gips VideoEngineLinux has the Linux specific implementation
 *	
 *
 */


#ifndef PUBLIC_GIPS_VIDEO_ENGINE_LINUX_H
#define PUBLIC_GIPS_VIDEO_ENGINE_LINUX_H

#include "GipsVideoEngine.h"

class GIPSXWindowsErrorHandler
{
public:
    virtual int Error(Display *display, XErrorEvent *theEvent) = 0;
    virtual ~GIPSXWindowsErrorHandler() {};
};

class VIDEOENGINE_DLLEXPORT GipsVideoEngineLinux : public GipsVideoEngine
{
public:
	virtual int GIPSVideo_AddLocalRenderer(Window renderWin, float left = 0.0, float top = 0.0, float right = 1.0, float bottom = 1.0) = 0;
	virtual int GIPSVideo_AddRemoteRenderer(int channel, Window remoteWin, float left = 0.0, float top = 0.0, float right = 1.0, float bottom = 1.0) = 0;
    	virtual int GIPSVideo_RegisterXWindowsErrorHandlerCallback( GIPSXWindowsErrorHandler* obj) = 0;

	virtual ~GipsVideoEngineLinux();
};


VIDEOENGINE_DLLEXPORT GipsVideoEngineLinux &GetGipsVideoEngine();
VIDEOENGINE_DLLEXPORT GipsVideoEngineLinux* GetNewVideoEngine();

#endif

