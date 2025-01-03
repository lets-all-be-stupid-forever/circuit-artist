#ifndef PROFILER_H
#define PROFILER_H

// Resets stats. Needs to be called begin of each frame.
void ProfilerReset();

// Profiling functions that are called every frame.
// The statistics is the running average.
void ProfilerTic(const char* name);
void ProfilerTac();

// Profiling single-call functions, that are not called every frame.
void ProfilerTicSingle(const char* name);
void ProfilerTacSingle(const char* name);

// Draws profiling stats on screen.
void ProfilerDraw();

#endif
