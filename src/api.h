#ifndef CA_API_H
#define CA_API_H
#include "img.h"
#include "rendering.h"
#include "sim.h"
#include "sprite.h"

// Describes the list of all Levels and Tutorial items available.
typedef struct {
  // Title of each Tutorial chapter.
  const char* help_name[50];
  // Multi-line text of each tutorial chapter (rendered with
  // DrawTextBoxAdvanced).
  const char* help_txt[50];
  // Images associated to each tutorial chapter.
  Sprite help_sprites[50][20];
  // Descriptor of each Level.
  struct {
    // Index of the level.
    int ilevel;
    // Legend of each level displayed when hovering in level selection window.
    const char* name;
    // Detailed text string for the description of each level.
    // The strings have a special format, allowing "bold" strings and embedded
    // images. It is rendered by DrawTextBoxAdvanced, you can look at its doc
    // for more details on the format.
    const char* desc;
    // Icon sprite of the level (displayed in the selection window)
    Sprite icon;
    // Sprites associated with the desc string.
    Sprite sprites[20];
    // Flag when level is complete
    bool complete;
  } options[60];
  // Image that is loaded on the game startup
  char* startup_image_path;
} LevelOptions;

// Detailed low level description of a Level.
typedef struct {
  // Index of the level.
  int ilevel;
  // Number of simulation external components.
  int num_components;
  // Descriptor for the pins (one for each component).
  PinDesc* pindesc;
  // External component instance for each component.
  ExtComp* extcomps;
} LevelDesc;

// Result of the call to ApiOnLevelTick
typedef struct {
  // Whether clock has been updated.
  bool clock_updated;
  // Whether the power_on_reset is on.
  bool reset;
  // Value of the clock.
  int clock_value;
  // Count of the clock (is displayed in the screen).
  int clock_count;
} TickResult;

// Initializes things in lua.
// The first script it calls is the luasrc/app.lua, the entrypoint to the lua
// scripts.
void ApiLoad();

// Shuts down the lua context.
void ApiUnload();

// Loads a level by index.
void ApiLoadLevel(int i);

// Sends the start simulation event to lua.
void ApiStartLevelSimulation();

// Sends the stop simulation event to lua.
void ApiStopLevelSimulation();

// Sets the clock rate.
void ApiSetClockTime(float clock_rate);

// Asks the Level to draw to a dedicated render texture.
void ApiOnLevelDraw(RenderTexture2D target, float camera_x, float camera_y,
                    float camera_spacing);

// Clock event sent to lua.
// Sent whenever a clock has been updated so the componets can sync their
// internal memory.
void ApiLevelClock(int ilevel, int* inputs, bool reset);

// Main component update entrypoint, which triggers an update component in lua.
// Called from the simulator during the simulation algorithm.
void ApiUpdateLevelComponent(void* ctx, int ic, int* prev_in, int* next_in,
                             int* output);

// Low-level tick event. Called in a fixed high frequency rate.
// Used mostly by the clock component, but can also be used in random
// components to handle things like keyboard events (mind though that it can be
// called multiple times per frame).
TickResult ApiOnLevelTick(float dt);

// Getter for the level descriptor of the active level. (set by the ApiLoadLevel
// function)
LevelDesc* ApiGetLevelDesc();

// Getter for the global list of all available levels and tutorial pages.
LevelOptions* ApiGetLevelOptions();

#endif
