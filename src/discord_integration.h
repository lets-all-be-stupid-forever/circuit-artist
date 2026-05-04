#pragma once

#ifdef WITH_STEAM

#ifdef __cplusplus
extern "C" {
#endif

void discord_init(void);
void discord_shutdown(void);
void discord_run_callbacks(void);
void discord_set_editing(const char* filename, const char* level_name, int num_nands);
void discord_set_simulating(const char* filename, const char* level_name, int num_nands);

#ifdef __cplusplus
}
#endif

#else

#define discord_init()                          ((void)0)
#define discord_shutdown()                      ((void)0)
#define discord_run_callbacks()                 ((void)0)
#define discord_set_editing(f, l, n)            ((void)0)
#define discord_set_simulating(f, l, n)         ((void)0)

#endif
