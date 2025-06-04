#  Model O Auto-Clicker 


## Requirements

- macOS with CoreGraphics framework
- Xcode Command Line Tools
- Accessibility permissions for Terminal

## Installation

1. Clone this repository:
```bash
git clone <repository-url>
cd enhanced-model-o-autoclicker
```

2. Compile the program:
```bash
gcc -o autoclicker model_o_final.c -framework CoreGraphics -lpthread
```

3. Grant accessibility permissions:
   - Go to **System Preferences** → **Security & Privacy** → **Privacy** → **Accessibility**
   - Click the lock icon and enter your password
   - Add **Terminal** to the list of allowed applications

## Usage

1. Run the program:
```bash
./autoclicker
```

2. **Toggle auto-clicker**: Press the scroll wheel (middle button) or side button to enable/disable

3. **Adjust click rate**: When enabled, use the scroll wheel to control speed:
   - **Scroll DOWN** = INCREASE click rate
   - **Scroll UP** = DECREASE click rate

4. **Start clicking**: Hold down the left mouse button to begin auto-clicking

5. **Stop clicking**: Release the left mouse button to pause

6. **Exit**: Press `Ctrl+C` to quit the program

## Configuration

You can modify these settings in the source code:

```c
#define MIN_CLICKS_PER_SECOND 1     // Minimum click rate
#define MAX_CLICKS_PER_SECOND 13    // Maximum click rate  
#define CLICK_INCREMENT 1           // Rate change per scroll
#define RANDOM_VARIATION 0.1        // 10% timing variation
#define TOGGLE_BUTTON1 2            // Middle button (scroll wheel)
#define TOGGLE_BUTTON2 3            // Side button
```

## Status Display

The program shows a live status line:
```
Status: ENABLED | Click rate: 8 CPS
```

- **ENABLED/DISABLED**: Current toggle state
- **Click rate**: Current clicks per second setting



## Technical Details

- **Language**: C
- **Framework**: CoreGraphics (macOS)
- **Threading**: POSIX threads (pthread)
- **Event Handling**: CGEventTap for system-wide mouse monitoring
- **Synchronization**: Mutex locks for thread safety


**Click rate feels inconsistent:**
- This is intentional - random variation mimics human clicking
- Adjust `RANDOM_VARIATION` if needed (0.0 = no variation, 0.2 = 20% variation)

**Mouse buttons not working:**
- Check button assignments in the configuration section
- Your mouse buttons might use different numbers


## Disclaimer

This tool is intended for legitimate automation purposes. Users are responsible for ensuring compliance with the terms of service of any software they use this with. The authors are not responsible for any consequences of using this software.
