#include <QWidget>
#import <Cocoa/Cocoa.h>

extern "C" void changeTitlebarColor(WId winId, double red, double green, double blue, double alpha){
    if (winId == 0) return;
    NSView* view = (NSView*)winId;
    NSWindow* window = [view window];
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:red green:green blue:blue alpha: alpha];
}
