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

WindowManager::WindowManager() :
    _cocoaRenderView1(nil), _cocoaRenderView2(nil)
{

}

WindowManager::~WindowManager()
{
    if (_cocoaRenderView1)
    {
        [    _cocoaRenderView1 release];
    }
    if(_cocoaRenderView2)
    {
        [_cocoaRenderView2 release];
    }
}

int WindowManager::CreateWindows(void* window1Title,
                                            void* window2Title)
{
    NSRect outWindow1Frame = NSMakeRect(352,288,600,400);
    NSWindow* outWindow1 = [[NSWindow alloc] initWithContentRect:outWindow1Frame
                            styleMask:NSTitledWindowMask
                            backing:NSBackingStoreBuffered defer:NO];
    [outWindow1 orderOut:nil];
    NSRect cocoaRenderView1Frame = NSMakeRect(0, 0, 600,400); 
    _cocoaRenderView1 = [[CocoaRenderView alloc]
                          initWithFrame:cocoaRenderView1Frame];
    [[outWindow1 contentView] addSubview:_cocoaRenderView1];
    [outWindow1 setTitle:[NSString stringWithFormat:@"%s", window1Title]];
    [outWindow1 makeKeyAndOrderFront:NSApp];

    NSRect outWindow2Frame = NSMakeRect(352,300,600,400);
    NSWindow* outWindow2 = [[NSWindow alloc] initWithContentRect:outWindow2Frame
                            styleMask:NSTitledWindowMask
                            backing:NSBackingStoreBuffered defer:NO];
    [outWindow2 orderOut:nil];
    NSRect cocoaRenderView2Frame = NSMakeRect(0, 0, 600,400);
    _cocoaRenderView2 = [[CocoaRenderView alloc]
                          initWithFrame:cocoaRenderView2Frame];
    [[outWindow2 contentView] addSubview:_cocoaRenderView2];
    [outWindow2 setTitle:[NSString stringWithFormat:@"%s", window2Title]];
    [outWindow2 makeKeyAndOrderFront:NSApp];

    return 0;
}

int WindowManager::TerminateWindows()
{
    return 0;
}

void* WindowManager::GetWindow1()
{
    return _cocoaRenderView1;
}

void* WindowManager::GetWindow2()
{
    return _cocoaRenderView2;
}

bool WindowManager::SetTopmostWindow()
{
    return true;
}

int main(int argc, const char * argv[])
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    [NSApplication sharedApplication];

    // we have to run the test in a secondary thread because we need to run a
    // runloop, which blocks
    TestClass* autoTestClass = [[TestClass alloc]init];
        [NSThread detachNewThreadSelector:@selector(autoTestWithArg:)
         toTarget:autoTestClass withObject:nil];

	// process OS events. Blocking call
	[[NSRunLoop mainRunLoop]run];
	[pool release];
}

@implementation TestClass

-(void)autoTestWithArg:(NSString*)answerFile;
{

    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    TestMain autoTest;
    int success = autoTest.BeginOSIndependentTesting();
    [pool release];
    return;
}

@end


