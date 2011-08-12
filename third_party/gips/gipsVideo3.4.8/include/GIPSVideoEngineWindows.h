/**
*	GipsVideoEngineWindows.h
*	Created by: Patrik Westin
*	Date      : 2005-05-09
*
*	Public API for GIPS VideoEngine on a Windows PC platform.
* 
*	Copyright (c) 2005
*	Global IP Sound AB, Organization number: 5565739017
*	Ölandsgatan 42, SE-116 63 Stockholm, Sweden
*	All rights reserved.
*
*	Note: Gips VideoEngineWindows has the Windows specific implementation
*	
**/

#ifndef PUBLIC_GIPS_VIDEO_ENGINE_WINDOWS_H
#define PUBLIC_GIPS_VIDEO_ENGINE_WINDOWS_H

#include "GipsVideoEngine.h"
#include <guiddef.h>
#include <strmif.h>
#include <d3d9.h>

class VIDEOENGINE_DLLEXPORT GipsVideoEngineWindows : public GipsVideoEngine
{
public:
    virtual int GIPSVideo_SelectBestCamera() = 0;

    virtual int GIPSVideo_SetCaptureCardProperties(int width, int height, int frameRate = -1, bool interlaced = false) = 0;
    virtual int GIPSVideo_ViewCaptureDialogBox(const char* dialogTitle, HWND m_hWnd = NULL, unsigned int x=200, unsigned int y=200) = 0;
	virtual int GIPSVideo_SetBackgroundImage(int channel, HBITMAP bitMap, unsigned int time = 0) = 0;

    virtual int GIPSVideo_AddLocalRenderer(HWND, int zOrder, float left, float top, float right, float bottom) = 0; 
    virtual int GIPSVideo_AddRemoteRenderer(int channel, HWND, int zOrder, float left, float top, float right, float bottom) = 0; 
	virtual int GIPSVideo_SetCropping(int channel,unsigned char streamID, float left, float top, float right, float bottom) = 0;

	/**
	*	DirectDraw rendering
	*/
    virtual int GIPSVideo_EnableDirectDraw(bool enable) = 0;
	virtual int GIPSVideo_EnableTransparentBackground(HWND hWnd, bool enable) = 0;
	virtual int GIPSVideo_AddFullScreenRender(HWND hWnd) = 0;
	virtual int GIPSVideo_AddRemoteDemuxRender(int channel, unsigned char streamID, HWND hWnd, int zOrder, float left, float top, float right, float bottom) = 0 ;
	virtual int GIPSVideo_AddRenderBitmap(HWND hWnd, unsigned char pictureId, HBITMAP bitMap, float left, float top, float rigth, float bottom, DDCOLORKEY* transparentColorKey = 0) = 0;
    virtual int GIPSVideo_AddRenderText(HWND hWnd, unsigned char textId, const char* text, int textLength, COLORREF colorText, COLORREF colorBg, float left, float top, float right, float bottom, bool transparent) = 0;
	virtual int GIPSVideo_ConfigureRender(int channel, unsigned char streamID, HWND hWnd, int zOrder, float left, float top, float right, float bottom) = 0;

	/**
	*	DirectShow rendering
	*/

	/**
	*	Direct3D rendering
	*/
	virtual int GIPSVideo_EnableDirect3D(bool enable) = 0;
    virtual LPDIRECT3DSURFACE9 GIPSVideo_GetD3DSurface(HWND hWnd)=0;

	// enables VMR, VMR9 or EVR
	virtual int GIPSVideo_EnableMixingRender(bool enable) = 0;

	// configures the render
	virtual int GIPSVideo_ConfigureMixer(int channel, char streamID, HWND hWnd, int zOrder, float alpha, float left, float top, float right, float bottom) = 0;

	// change rendering hwnd without stop
	virtual int GIPSVideo_ChangeHWND(int channel,HWND hWnd, HWND oldhWnd) = 0;

	// windows messages
	virtual int GIPSVideo_OnPaint(HDC hdc) = 0;
	virtual int GIPSVideo_OnSize(HWND hWnd) = 0;
	virtual int GIPSVideo_OnDisplayMode() = 0;

	// Get a handle to the VMR filter
	virtual int GIPSVideo_GetAssociatedRenderFilter(HWND hWnd, IBaseFilter** filter) = 0;

	// Other render than the default Microsoft VMR filter 
    virtual int GIPSVideo_AddLocalRenderer(REFCLSID) = 0;
    virtual int GIPSVideo_AddLocalRenderer(IBaseFilter*) = 0; 
    virtual int GIPSVideo_AddRemoteRenderer(int channel, REFCLSID) = 0;
    virtual int GIPSVideo_AddRemoteRenderer(int channel, IBaseFilter*) = 0; 

	// Require Microsoft VMR filter
	virtual int GIPSVideo_GetSnapShot(HWND hWnd,LPBITMAPINFOHEADER header, int size) = 0;  

	virtual ~GipsVideoEngineWindows();
};

VIDEOENGINE_DLLEXPORT GipsVideoEngineWindows &GetGipsVideoEngine();

VIDEOENGINE_DLLEXPORT GipsVideoEngineWindows* GetNewVideoEngine();

VIDEOENGINE_DLLEXPORT void DeleteGipsVideoEngine(GipsVideoEngine *videoEngine);

#endif
