#ifndef VERSION_H
#define VERSION_H
#define CA_VERSION "v1.0.5 - 07Oct2024"

// v1.0.5 -->
//   - Changed complete level icons
//   - Splitted sandbox in sandbox and custom component levels.
//   - Updated some icons (flag)
//   - Updated UI of line tool: separation width has a dedicated button like the
//   text tool.
//   - Moved tutorial initial image to the tutorial section as TLDR section.
//   - Removed dependency on levels so user can pick the one he wants directly.
//   - Added a DEMO version with limited image size (256) to make freely
//   available.
// v1.0.4 --> v1.0.4: Level Progression
//   - Added Level Progression and Steam Achievements.
//   - Added 2 dark colors to default palette
//   - Added feature to save and load a selection (to mitigate lack of blueprint
//   support)
//   - Added possibility to change spacing between consecutive lines in line
//   tool.
//   - Removed minimap and moved sidebar buttons to topbar
//   - Added number of pixels to selection on status bar
//   - Rename circuitopedia to "Tutorial"
//   - Fix bug in SRLATCH image schema
//   - Fix a bug in 9 picture in the 7seg level
//   - Fix crash when window too small
// v1.0.3 --> v1.0.4: Improved Lua Support
//  - (QoL) Loads lua scripts from luasrc/scripts folder.
//          Improved Sandbox level with Keyboard + ROM + RAM + Display
//          components for reference.
//  - (QoL) Added console launcher version for windows.
//
// v1.0.2 --> v1.0.3: Small QoL
//  -   (QoL) Window starts maximized. Initial window is a bit smaller, to make
//        it work better at smaller screens. Dialogs change size with screen
//        size (to adapt for smaller screens).
//  -   (QoL) Can pick color using ALT during brush, line and bucket tools.
//  -   (Others) The parsing algorithm requires that all nands have 2 inputs and
//         1 output, otherwise it enters an error state.
//  -   (Bugfix) Fixed tilted rectangle display in levels.
//
// v1.0.1 --> v1.0.2: Bugfix
//    - (Bugfix) Pasted Image Ugly.
//            Pasting images that were not encoded with 32 bits were not pasted
//            propperly. It would happen specially when the saved image went
//            through a website that cropped the alpha channel.
//    - (Bugfix) Picker on Background Was Picking Transparent Color.
//            The picker on background was picking transparent color because
//            internally it is represented as such. The problem is that you
//            then couldn't use that color to "draw" black pixels. Fixed it by
//            making it be black pixels instead.
//    - (Improvement): Hotkeys on Selection.
//            Improved legend of selection tool to hotkeys not mentioned
//            anywhere like arrows, pressing CTRL before dragging, pressing
//            SHIFT while dragging etc
//    - (Bugfix) OpenMP is taking all machine's CPU.
//            It seems windows' openmp is doing busy wait on the openmp
//            threads, leading to a 100% cpu usage all the time, even when it
//            is very light on calculations. To remediate that we put a max of
//            6 threads for openmp, until we find a better solution.
#endif
