/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef TEST_MAC_COCOA_H_
#define TEST_MAC_COCOA_H_

#if defined __APPLE__

#define MAC_COCOA_USE_NSRUNLOOP 1

#import <Cocoa/Cocoa.h>
#import "cocoa_render_view.h"
#include <string>

//added
using namespace std;

class WindowManager
{
public:
    WindowManager();
    ~WindowManager();
    //virtual void* GetWindow1();
    //virtual void* GetWindow2();
	//added 
	virtual void* GetVideoWindow();
	
	virtual int InitializeWindows();
	//end added
    //virtual int CreateWindows(void* window1Title,
      //                        void* window2Title);
    //virtual int TerminateWindows();
    //virtual bool SetTopmostWindow();
	
protected:
	virtual CocoaRenderView* CreateView(NSString* title, NSRect windowDimensions, NSRect viewDimensions);

private:
	
  //  CocoaRenderView* _cocoaRenderView1;
  //  CocoaRenderView* _cocoaRenderView2;
	//added
//	CocoaRenderView* _consoleView;
    CocoaRenderView* _videoView;
//	CocoaRenderView* _outputView;
};

@interface TestClass : NSObject
{
	
}

-(void)autoTestWithArg:(NSString*)answerFile;;

@end

#endif
#endif  // TEST_MAC_COCOA_H_
