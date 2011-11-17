/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Cary Bran <cbran@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

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
	
	NSString* filePath= NULL;
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


