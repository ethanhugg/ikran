/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include	"test_mac_cocoa.h"
#include	"test_main.h"

WindowManager::WindowManager() : _videoView(nil){

}

WindowManager::~WindowManager(){
  	if(_videoView){
        [_videoView release];
    }
}

/**
 * Creates a cocoa video view using the webrtc CocoaRenderView object
 */
CocoaRenderView* WindowManager::CreateView(NSString* title, NSRect windowDimensions, NSRect viewDimensions){

	NSWindow* window = [[NSWindow alloc] initWithContentRect:windowDimensions
						styleMask:NSTitledWindowMask
						backing:NSBackingStoreBuffered defer:NO];
	
	CocoaRenderView* view = [[CocoaRenderView alloc]initWithFrame:viewDimensions];
    [[window contentView] addSubview:view];
    [window setTitle:title];
	[window makeKeyAndOrderFront:NSApp];
	
	return view;
}


void* WindowManager::GetVideoWindow(){
	if(_videoView){
		InitializeWindow();
	}
	return _videoView;
}

int WindowManager::InitializeWindow(){
	if(_videoView){
		return 0;
	}
	
	NSString* videoTitle = @"Caller Video Window";
	
	_videoView =   CreateView(videoTitle, NSMakeRect(352,288,600,400),  NSMakeRect(0, 0, 600,400));
	
	return 0;
}



int main(int argc, const char * argv[]){
		
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    [NSApplication sharedApplication];
	
	NSString* filePath;
	if (argc == 2) {
		//assume that the first arguement is the path to the test file
		filePath = [[NSString alloc] initWithCString:argv[1]];
	}
	

    // we have to run the test in a secondary thread because we need to run a
    // runloop, which blocks
    TestClass* autoTestClass = [[TestClass alloc]init];
        [NSThread detachNewThreadSelector:@selector(autoTestWithArg:)
         toTarget:autoTestClass withObject:filePath];

	// process OS events. Blocking call
	[[NSRunLoop mainRunLoop]run];
	[pool release];
}

@implementation TestClass

-(void)autoTestWithArg:(NSString*)configPath;
{
	
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	string path;
	TestMain autoTest;
	if (configPath) {
		path = [configPath cStringUsingEncoding:[NSString defaultCStringEncoding]];
	}
	 
	int result = autoTest.BeginOSIndependentTesting(path);
	
	[pool release];
    return;
}

@end


