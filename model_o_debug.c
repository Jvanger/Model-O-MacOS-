#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <CoreGraphics/CoreGraphics.h>

// Global variables
static bool autoClickEnabled = false;
static CFMachPortRef eventTap;
static CFRunLoopSourceRef runLoopSource;

// Function to simulate a mouse click at the current position
void simulateClick() {
    CGEventRef mouseDown = CGEventCreateMouseEvent(
        NULL, kCGEventLeftMouseDown,
        CGEventGetLocation(CGEventCreate(NULL)),
        kCGMouseButtonLeft
    );
    
    CGEventRef mouseUp = CGEventCreateMouseEvent(
        NULL, kCGEventLeftMouseUp,
        CGEventGetLocation(CGEventCreate(NULL)),
        kCGMouseButtonLeft
    );
    
    CGEventPost(kCGHIDEventTap, mouseDown);
    usleep(10000);  // Short delay between down and up (10ms)
    CGEventPost(kCGHIDEventTap, mouseUp);
    
    CFRelease(mouseDown);
    CFRelease(mouseUp);
}

// Auto-clicking thread function
void* autoClickThread(void* arg __attribute__((unused))) {
    while (true) {
        if (autoClickEnabled) {
            simulateClick();
            // Random delay between 60-80ms (around 14 clicks per second)
            usleep(60000 + (rand() % 20000));
        } else {
            // When disabled, sleep to avoid wasting CPU
            usleep(100000);  // 100ms
        }
    }
    return NULL;
}

// Callback function for the event tap
CGEventRef eventCallback(CGEventTapProxy proxy __attribute__((unused)), 
                        CGEventType type, 
                        CGEventRef event, 
                        void* userInfo __attribute__((unused))) {
    
    // Debug - print the event type
    printf("Event type: %d\n", (int)type);
    
    // Mouse events: 
    // 1 = left down, 2 = left up, 3 = right down, 4 = right up, 
    // 5 = mouse moved, 6 = left dragged, 7 = right dragged,
    // 25 = other mouse down, 26 = other mouse up
    
    if (type == kCGEventOtherMouseDown || 
        type == kCGEventLeftMouseDown || 
        type == kCGEventRightMouseDown) {
        
        // Get button number
        CGMouseButton button = (CGMouseButton)CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
        
        // Print button number for debugging
        printf("Mouse button pressed: %d\n", (int)button);
        
        // Button 0 = left, 1 = right, 2 = middle/scroll wheel, 3+ = extra buttons
        if (button == 2) {  // Middle button (scroll wheel)
            // Toggle auto-clicking state
            autoClickEnabled = !autoClickEnabled;
            
            printf("Auto-clicking %s\n", autoClickEnabled ? "ENABLED" : "DISABLED");
            
            // Don't consume the event - let it pass through
            // return NULL;
        }
    }
    
    // Allow all events to pass through
    return event;
}

int main() {
    // Seed the random number generator
    srand((unsigned int)time(NULL));
    
    // Create event tap to monitor ALL mouse button presses
    CGEventMask eventMask = 
        CGEventMaskBit(kCGEventLeftMouseDown) | 
        CGEventMaskBit(kCGEventRightMouseDown) | 
        CGEventMaskBit(kCGEventOtherMouseDown);
    
    printf("Starting Model O Auto-Clicker Debug Version\n");
    printf("This program will print the button number when you press any mouse button\n");
    printf("Press Ctrl+C to exit\n\n");
    
    eventTap = CGEventTapCreate(
        kCGSessionEventTap,     // Tap at session level
        kCGHeadInsertEventTap,  // Insert at head 
        kCGEventTapOptionListenOnly,  // Just listen, don't modify events
        eventMask,
        eventCallback,
        NULL
    );
    
    if (!eventTap) {
        fprintf(stderr, "Failed to create event tap. Make sure your app has accessibility permissions.\n");
        fprintf(stderr, "Go to System Preferences > Security & Privacy > Privacy > Accessibility\n");
        fprintf(stderr, "Add Terminal to the list of allowed applications.\n");
        return 1;
    }
    
    // Create a run loop source and add it to the current run loop
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    
    // Enable the event tap
    CGEventTapEnable(eventTap, true);
    
    // Create and start auto-clicking thread
    pthread_t clickThread;
    if (pthread_create(&clickThread, NULL, autoClickThread, NULL) != 0) {
        fprintf(stderr, "Failed to create auto-clicking thread.\n");
        return 1;
    }
    
    // Run the loop to process events
    CFRunLoopRun();
    
    // Cleanup
    if (runLoopSource) {
        CFRelease(runLoopSource);
    }
    
    if (eventTap) {
        CFRelease(eventTap);
    }
    
    return 0;
}