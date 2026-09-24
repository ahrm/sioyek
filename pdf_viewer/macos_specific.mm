#include <AppKit/AppKit.h>
#include <QWidget>
#import <Cocoa/Cocoa.h>

extern "C" void showLookupForString(WId winId, const char* text, double x, double y) {
    if (winId == 0 || text == nullptr) return;

    NSView* view = (NSView*)winId;
    NSString* nsText = [NSString stringWithUTF8String:text];

    NSPoint point = NSMakePoint(x, y);

    [view showDefinitionForAttributedString:[[NSAttributedString alloc] initWithString:nsText]
                                    atPoint:point];
}

extern "C" void changeTitlebarColor(WId winId, double red, double green, double blue, double alpha){
    if (winId == 0) return;
    NSView* view = (NSView*)winId;
    NSWindow* window = [view window];
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:red green:green blue:blue alpha: alpha];
}
