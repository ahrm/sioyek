#include <AppKit/AppKit.h>
#include <QWidget>
#import <Cocoa/Cocoa.h>

extern "C" void changeTitlebarColor(WId winId, double red, double green, double blue, double alpha){
    if (winId == 0) return;
    NSView* view = (NSView*)winId;
    NSWindow* window = [view window];
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:red green:green blue:blue alpha: alpha];
}

@interface DraggableTitleView : NSView
@end

@implementation DraggableTitleView

// Handle mouse click events
- (void)mouseDown:(NSEvent *)event {
    // double-click to zoom
    if ([event clickCount] == 2) {
        [self.window zoom:nil];
    } else {
        // drag Window
        [self.window performWindowDragWithEvent:event];
    }
}

- (void)updateTrackingAreas {
    [self initTrackingArea];
}

-(void) initTrackingArea {
    NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect |
            NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);

    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[self bounds]
        options:options
        owner:self
        userInfo:nil];

    [self addTrackingArea:area];
}
-(void)mouseEntered:(NSEvent *)event {
    if((self.window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [[self.window standardWindowButton: NSWindowCloseButton] setHidden:NO];
        [[self.window standardWindowButton: NSWindowMiniaturizeButton] setHidden:NO];
        [[self.window standardWindowButton: NSWindowZoomButton] setHidden:NO];
    }
}

-(void)mouseExited:(NSEvent *)event {
    if((self.window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [[self.window standardWindowButton: NSWindowCloseButton] setHidden:YES];
        [[self.window standardWindowButton: NSWindowMiniaturizeButton] setHidden:YES];
        [[self.window standardWindowButton: NSWindowZoomButton] setHidden:YES];
    }
}

@end

extern "C" void hideWindowTitleBar(WId winId) {
    if (winId == 0) return;

    NSView* nativeView = reinterpret_cast<NSView*>(winId);
    NSWindow* nativeWindow = [nativeView window];

    if(nativeWindow.titleVisibility == NSWindowTitleHidden){
        return;
    }

    [[nativeWindow standardWindowButton: NSWindowCloseButton] setHidden:YES];
    [[nativeWindow standardWindowButton: NSWindowMiniaturizeButton] setHidden:YES];
    [[nativeWindow standardWindowButton: NSWindowZoomButton] setHidden:YES];
    NSRect contentViewBounds = nativeWindow.contentView.bounds;

    DraggableTitleView *titleBarView = [[DraggableTitleView alloc] initWithFrame:NSMakeRect(0, 0, contentViewBounds.size.width, 22)];
    titleBarView.autoresizingMask = NSViewWidthSizable;
    titleBarView.wantsLayer = YES;
    titleBarView.layer.backgroundColor = [[NSColor clearColor] CGColor];

    [nativeWindow.contentView addSubview:titleBarView];

    [nativeWindow setTitleVisibility:NSWindowTitleHidden];
    [nativeWindow setStyleMask:[nativeWindow styleMask] | NSWindowStyleMaskFullSizeContentView];
    [nativeWindow setTitlebarAppearsTransparent:YES];
}
