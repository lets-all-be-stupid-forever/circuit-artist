#ifndef CA_EVENT_QUEUE
#define CA_EVENT_QUEUE
#include "common.h"

typedef struct {
  int socket;
  int value;
} SocketEvent;

typedef struct {
  SocketEvent** q;     // one event queue per time.
  int* q_cap;          // capacity of each queue
  int* qsize;          // size of each queue
  int cur_time;        // current time
  int ntime;           // number of time steps
  int pending_events;  // number of events to be resolved
} EventQueue;

void event_queue_init(EventQueue* e, int ntime);
void event_queue_destroy(EventQueue* e);
void event_queue_step_forward(EventQueue* e);
void event_queue_step_backward(EventQueue* e);
void event_queue_get_current_events(EventQueue* e, int* nEvent,
                                    SocketEvent** pEvents);
void event_queue_schedule(EventQueue* e, int dt, int socket, int value);
void event_queue_unschedule(EventQueue* e, int dt);

#endif
