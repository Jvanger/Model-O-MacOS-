#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <CoreGraphics/CoreGraphics.h>

// Configuration
#define MIN_CLICKS_PER_SECOND 13
#define MAX_CLICKS_PER_SECOND 25
#define CLICK_INCREMENT 1  // How much to increase/decrease per scroll
#define RANDOM_VARIATION 0.1  // 10% variation
#define RAMP_UP_DURATION_MS 50  // Time to ramp up to full speed in milliseconds
#define RAMP_UP_STEPS 4  // Number of steps during ramp-up

// Define toggle buttons - change these to match your mouse
#define TOGGLE_BUTTON1 2  // Middle button (scroll wheel)
#define TOGGLE_BUTTON2 3  // Side button

// Global variables
static bool toggleEnabled = false;  // Toggle for the auto-clicker feature
static bool autoClickActive = false;  // Whether auto-clicking is actively happening
static bool leftMouseHeld = false;  // Whether left mouse button is being held
static bool firstClick = true;  // First click in a sequence
static int currentClicksPerSecond = 13;  // Starting click rate
static CFMachPortRef eventTap;
static CFRunLoopSourceRef runLoopSource;
static pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;

// Function declarations
void updateStatusLine(void);
void updateClickRate(int delta);

// Function to generate random delay between clicks (in microseconds)
useconds_t generateRandomDelay(double speedFactor) {
    // Base delay in microseconds (1/currentClicksPerSecond of a second)
    double baseDelay = 1000000.0 / (currentClicksPerSecond * speedFactor);
    
    // Random factor between (1-RANDOM_VARIATION) and (1+RANDOM_VARIATION)
    double randomFactor = 1.0 + RANDOM_VARIATION * (2.0 * ((double)rand() / RAND_MAX) - 1.0);
    
    // Final delay with random variation
    return (useconds_t)(baseDelay * randomFactor);
}

// Function to simulate a mouse click at the current position
void simulateClick() {
    // Create a mouse up event to release the current click
    CGEventRef mouseUp = CGEventCreateMouseEvent(
        NULL, kCGEventLeftMouseUp,
        CGEventGetLocation(CGEventCreate(NULL)),
        kCGMouseButtonLeft
    );
    
    // Create a mouse down event for the next click
    CGEventRef mouseDown = CGEventCreateMouseEvent(
        NULL, kCGEventLeftMouseDown,
        CGEventGetLocation(CGEventCreate(NULL)),
        kCGMouseButtonLeft
    );
    
    // Post the events
    CGEventPost(kCGHIDEventTap, mouseUp);
    usleep(10000);  // Short delay between up and down (10ms)
    CGEventPost(kCGHIDEventTap, mouseDown);
    
    // Release the events
    CFRelease(mouseUp);
    CFRelease(mouseDown);
}

// Function to perform a gradual ramp-up of clicking speed
void performRampUp() {
    // Skip ramp-up if not the first click
    if (!firstClick) return;
    
    // Calculate time per step
    int msPerStep = RAMP_UP_DURATION_MS / RAMP_UP_STEPS;
    
    for (int i = 1; i <= RAMP_UP_STEPS; i++) {
        // Skip if toggle was disabled during ramp-up
        pthread_mutex_lock(&stateMutex);
        if (!toggleEnabled || !leftMouseHeld) {
            pthread_mutex_unlock(&stateMutex);
            return;
        }
        pthread_mutex_unlock(&stateMutex);
        
        // Calculate speed factor for this step (0.2 to 1.0)
        double speedFactor = 0.5 + (0.8 * i / RAMP_UP_STEPS);
        
        // Simulate click
        simulateClick();
        
        // Sleep for calculated delay
        usleep(generateRandomDelay(speedFactor));
    }
    
    // Ramp-up complete
    firstClick = false;
}

// Auto-clicking thread function
void* autoClickThread(void* arg __attribute__((unused))) {
    while (true) {
        pthread_mutex_lock(&stateMutex);
        bool shouldClick = toggleEnabled && leftMouseHeld;
        bool isFirstClick = firstClick;
        pthread_mutex_unlock(&stateMutex);
        
        if (shouldClick) {
            if (isFirstClick) {
                performRampUp();
            } else {
                simulateClick();
                usleep(generateRandomDelay(1.0));  // Full speed
            }
        } else {
            // When disabled, sleep to avoid wasting CPU
            usleep(50000);  // 50ms
        }
    }
    return NULL;
}

// Function to update click rate
void updateClickRate(int delta) {
    pthread_mutex_lock(&stateMutex);
    
    int newRate = currentClicksPerSecond + delta;
    
    // Clamp to valid range
    if (newRate < MIN_CLICKS_PER_SECOND) {
        newRate = MIN_CLICKS_PER_SECOND;
    } else if (newRate > MAX_CLICKS_PER_SECOND) {
        newRate = MAX_CLICKS_PER_SECOND;
    }
    
    // Only update if there's a change
    if (newRate != currentClicksPerSecond) {
        currentClicksPerSecond = newRate;
        updateStatusLine();
    }
    
    pthread_mutex_unlock(&stateMutex);
}

// Function to update the status line
void updateStatusLine() {
    printf("\rStatus: %s | Click rate: %d CPS", 
           toggleEnabled ? "ENABLED " : "DISABLED", 
           currentClicksPerSecond);
    fflush(stdout);
}

// Callback function for the event tap
CGEventRef eventCallback(CGEventTapProxy proxy __attribute__((unused)), 
                        CGEventType type, 
                        CGEventRef event, 
                        void* userInfo __attribute__((unused))) {
    
    pthread_mutex_lock(&stateMutex);
    
    // Handle scroll wheel events (only when toggle is enabled)
    if (type == kCGEventScrollWheel && toggleEnabled) {
        // Get scroll delta (positive = scroll up, negative = scroll down)
        int64_t scrollDelta = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);
        
        if (scrollDelta > 0) {
            // Scroll up - decrease click rate (inverted)
            pthread_mutex_unlock(&stateMutex);
            updateClickRate(-CLICK_INCREMENT);
            pthread_mutex_lock(&stateMutex);
        } else if (scrollDelta < 0) {
            // Scroll down - increase click rate (inverted)
            pthread_mutex_unlock(&stateMutex);
            updateClickRate(CLICK_INCREMENT);
            pthread_mutex_lock(&stateMutex);
        }
        
        // Consume the scroll event to prevent it from affecting other applications
        pthread_mutex_unlock(&stateMutex);
        return NULL;
    }
    // Handle toggle button presses (middle or side button)
    else if (type == kCGEventOtherMouseDown) {
        CGMouseButton button = (CGMouseButton)CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
        
        if (button == TOGGLE_BUTTON1 || button == TOGGLE_BUTTON2) {
            toggleEnabled = !toggleEnabled;
            updateStatusLine();
            
            // Reset first click flag when toggling
            if (toggleEnabled) {
                firstClick = true;
            }
        }
    }
    // Track left mouse button state
    else if (type == kCGEventLeftMouseDown) {
        leftMouseHeld = true;
        
        // If toggle is enabled and this is the first activation, print message
        if (toggleEnabled && !autoClickActive) {
            autoClickActive = true;
        }
    }
    else if (type == kCGEventLeftMouseUp) {
        leftMouseHeld = false;
        
        // If auto-clicking was active, print message
        if (autoClickActive) {
            autoClickActive = false;
            
            // Reset first click flag for next activation
            if (toggleEnabled) {
                firstClick = true;
            }
        }
    }
    
    pthread_mutex_unlock(&stateMutex);
    
    // Allow all events to pass through (except consumed scroll events)
    return event;
}

int main() {
    // Seed the random number generator
    srand((unsigned int)time(NULL));
    
    // Create event tap to monitor mouse button presses and releases
    CGEventMask eventMask = 
        CGEventMaskBit(kCGEventLeftMouseDown) | 
        CGEventMaskBit(kCGEventLeftMouseUp) | 
        CGEventMaskBit(kCGEventOtherMouseDown) |
        CGEventMaskBit(kCGEventScrollWheel);
    
    eventTap = CGEventTapCreate(
        kCGSessionEventTap,     // Tap at session level
        kCGHeadInsertEventTap,  // Insert at head (receive events before they reach apps)
        kCGEventTapOptionDefault,
        eventMask,
        eventCallback,
        NULL
    );
    
    if (!eventTap) {
        fprintf(stderr, "Failed to create event tap. Make sure your app has accessibility permissions in System Preferences.\n");
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
    
    printf("=======================================\n");
    printf("Enhanced Model O Auto-Clicker Started\n");
    printf("=======================================\n");
    printf("1. Press the scroll wheel (button %d) or side button (button %d) to toggle auto-clicking mode\n", TOGGLE_BUTTON1, TOGGLE_BUTTON2);
    printf("2. When enabled, hold left mouse button to start auto-clicking\n");
    printf("3. Use scroll wheel to adjust click rate (Range: %d-%d clicks/second)\n", MIN_CLICKS_PER_SECOND, MAX_CLICKS_PER_SECOND);
    printf("   - Scroll DOWN to INCREASE rate\n");
    printf("   - Scroll UP to DECREASE rate\n");
    printf("4. Auto-clicking will stop when left mouse button is released\n");
    printf("5. Auto-clicking speed will ramp up gradually\n");
    printf("6. Press Ctrl+C to exit program\n\n");
    
    // Display initial status
    updateStatusLine();
    printf("\n");
    
    // Run the loop to process events
    CFRunLoopRun();
    
    // Cleanup (this code won't normally be reached as CFRunLoopRun doesn't return)
    if (runLoopSource) {
        CFRelease(runLoopSource);
    }
    
    if (eventTap) {
        CFRelease(eventTap);
    }
    
    pthread_mutex_destroy(&stateMutex);
    
    return 0;
}