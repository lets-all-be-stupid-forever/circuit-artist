#include "discord_integration.h"

#ifdef WITH_STEAM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "discord_rpc.h"
#include "utils.h"

// Client ID from https://discord.com/developers/applications
// Create an application, go to "Rich Presence" and upload a "game_icon" asset.
#define DISCORD_APP_ID "1500838388562133115"

static int64_t s_start_time;

static void on_ready(const DiscordUser* user) {
  printf("[discord] Connected as %s\n", user->username);
}

static void on_disconnected(int errorCode, const char* message) {
  printf("[discord] Disconnected (%d): %s\n", errorCode, message);
}

static void on_errored(int errorCode, const char* message) {
  printf("[discord] Error (%d): %s\n", errorCode, message);
}

void discord_init(void) {
  s_start_time = (int64_t)time(NULL);

  DiscordEventHandlers handlers;
  memset(&handlers, 0, sizeof(handlers));
  handlers.ready = on_ready;
  handlers.disconnected = on_disconnected;
  handlers.errored = on_errored;

  Discord_Initialize(DISCORD_APP_ID, &handlers, 1, NULL);
}

void discord_shutdown(void) { Discord_Shutdown(); }

void discord_run_callbacks(void) { Discord_RunCallbacks(); }

static void update_presence(const char* details, const char* state) {
  DiscordRichPresence presence;
  memset(&presence, 0, sizeof(presence));
  presence.startTimestamp = s_start_time;
  presence.largeImageKey = "icon_discord2";
  presence.largeImageText = "Circuit Artist";
  presence.details = details;
  presence.state = state;
  Discord_UpdatePresence(&presence);
}

static void set_presence(const char* level_name, int num_nands) {
  char details[128];
  if (num_nands > 0) {
    static const char* phrases_tiny[] = {
        // 1-19
        "Just %s NANDs... for now",
        // "Starting with %s NANDs",
        // "A humble %s NANDs",
    };
    static const char* phrases_small[] = {
        // 20-99
        "Making art with %s NANDs",
        // "Sculpting %s NANDs",
    };
    static const char* phrases_mid[] = {
        // 100-999
        "%s NANDs of pure logic",
        // "Commanding %s NANDs",
        // "%s NANDs and counting",
    };
    static const char* phrases_large[] = {
        // 1K-100k
        // "Wielding %s NANDs",
        // "%s NANDs strong",
        "An army of %s NANDs",
    };
    static const char* phrases_huge[] = {
        // 100k+
        "%s NANDs. Absolute madness.",
        // "What have you done... %s NANDs",
        // "%s NANDs. Send help.",
    };
    const char** phrases;
    int num_phrases;
    if (num_nands < 20) {
      phrases = phrases_tiny;
      num_phrases = sizeof(phrases_tiny) / sizeof(phrases_tiny[0]);
    } else if (num_nands < 100) {
      phrases = phrases_small;
      num_phrases = sizeof(phrases_small) / sizeof(phrases_small[0]);
    } else if (num_nands < 1000) {
      phrases = phrases_mid;
      num_phrases = sizeof(phrases_mid) / sizeof(phrases_mid[0]);
    } else if (num_nands < 100000) {
      phrases = phrases_large;
      num_phrases = sizeof(phrases_large) / sizeof(phrases_large[0]);
    } else {
      phrases = phrases_huge;
      num_phrases = sizeof(phrases_huge) / sizeof(phrases_huge[0]);
    }
    snprintf(details, sizeof(details), phrases[rand() % num_phrases],
             pretty_number(num_nands));
  } else {
    snprintf(details, sizeof(details), "Making art");
  }

  char state[128];
  const char* state_ptr = NULL;
  if (level_name) {
    snprintf(state, sizeof(state), "Level: %s", level_name);
    state_ptr = state;
  }

  update_presence(details, state_ptr);
}

void discord_set_editing(const char* filename, const char* level_name,
                         int num_nands) {
  (void)filename;
  set_presence(level_name, num_nands);
}

void discord_set_simulating(const char* filename, const char* level_name,
                            int num_nands) {
  (void)filename;
  set_presence(level_name, num_nands);
}

#endif
