#include "event_queue.h"

#include "assert.h"
#include "stdlib.h"

void event_queue_init(EventQueue* e, int ntime) {
  e->ntime = ntime;
  e->q_cap = malloc(ntime * sizeof(int));
  e->qsize = malloc(ntime * sizeof(int));
  e->q = malloc(ntime * sizeof(SocketEvent*));
  e->pending_events = 0;
  for (int iTime = 0; iTime < ntime; iTime++) {
    e->q_cap[iTime] = 128;
    e->qsize[iTime] = 0;
    e->q[iTime] = malloc(e->q_cap[iTime] * sizeof(SocketEvent));
  }
}

void event_queue_unschedule(EventQueue* e, int dt) {
  int i = (e->cur_time + dt) % e->ntime;
  e->qsize[i]--;
  e->pending_events--;
}

void event_queue_schedule(EventQueue* e, int dt, int socket, int value) {
  int i = (e->cur_time + dt) % e->ntime;
  int qIdx = e->qsize[i]++;
  e->q[i][qIdx] = (SocketEvent){socket, value};
  e->pending_events++;
  if (e->qsize[i] + 10 > e->q_cap[i]) {
    e->q_cap[i] *= 2;
    e->q[i] = realloc(e->q[i], e->q_cap[i] * sizeof(SocketEvent));
  }
}

void event_queue_destroy(EventQueue* e) {
  free(e->q_cap);
  free(e->qsize);
  for (int i = 0; i < e->ntime; i++) {
    free(e->q[i]);
  }
  free(e->q);
}

void event_queue_get_current_events(EventQueue* e, int* nEvent,
                                    SocketEvent** pEvents) {
  int i = e->cur_time;
  *nEvent = e->qsize[i];
  *pEvents = e->q[i];
}

void event_queue_step_forward(EventQueue* e) {
  e->pending_events -= e->qsize[e->cur_time];
  e->qsize[e->cur_time] = 0;
  e->cur_time = (e->cur_time + 1) % e->ntime;
}

void event_queue_step_backward(EventQueue* e) {
  e->cur_time = (e->cur_time + e->ntime - 1) % e->ntime;
  assert(e->qsize[e->cur_time] == 0);
}

