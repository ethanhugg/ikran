//  GIPSCocoaRenderer.h 
//  GIPS_Cocoa_Demo
//
//  Created by Zakk Hoyt on 8/25/09.
//  Copyright 2009 Global IP Solutions. All rights reserved.
#ifndef __GIPSCocoaRenderer_h
#define __GIPSCocoaRenderer_h

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#import <OpenGL/OpenGL.h>

@interface GIPSCocoaRenderer : NSOpenGLView {
	NSOpenGLContext*	_nsOpenGLContext;
}

@property (nonatomic, retain)NSOpenGLContext* _nsOpenGLContext;

- (void)initGIPSCocoaRenderer:(NSOpenGLPixelFormat*)fmt;

@end

#endif // __GIPSCocoaRenderer_h
