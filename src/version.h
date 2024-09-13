#ifndef VERSION_H
#define VERSION_H
#define CA_VERSION "v1.0.2 - Sep2024"

// v1.0.1 --> v1.0.2:
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
