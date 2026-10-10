#ifndef CA_LOG_H
#define CA_LOG_H

/* Log for lua stuff */
typedef struct {
  char** lines;
  int num_lines;
  int cap;   /* lines capacity */
  int first; /* index of first line */
} SimpleLog;

void log_init(SimpleLog* l);
void log_add_text(SimpleLog* l, const char* txt);
void log_free(SimpleLog* l);
char* log_get_line(SimpleLog* l, int nb);
int log_get_num_lines(SimpleLog* l);

void lua_log_text(const char* txt);

void win_log_init();
void win_log_open();
void win_log_update();
void win_log_draw();

#endif
