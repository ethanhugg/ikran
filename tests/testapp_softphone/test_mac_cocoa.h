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
   	virtual void* GetVideoWindow();
	virtual int InitializeWindow();
	
protected:
	virtual CocoaRenderView* CreateView(NSString* title, NSRect windowDimensions, NSRect viewDimensions);

private:
    CocoaRenderView* _videoView;
};

@interface TestClass : NSObject
{
	
}

-(void)autoTestWithArg:(NSString* )configPath;

@end

#endif
#endif  // TEST_MAC_COCOA_H_
