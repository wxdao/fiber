#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "fiber.h"

// global variables

static f_env *this_f_env = NULL;
static int ava_fid = 0;

// methods

// assembly methods
extern void _fiber_switch(fe_ctx *dest, fe_ctx **src);

static unsigned int get_mill_timestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

static void append_fcbs(fcb **fcbs_head, fcb *new_fcb) {
  new_fcb->prev = NULL;
  new_fcb->next = NULL;
  if (*fcbs_head == NULL) {
    *fcbs_head = new_fcb;
  } else {
    fcb *last = *fcbs_head;
    for (; last->next != NULL; last = last->next)
      ;
    new_fcb->prev = last;
    last->next = new_fcb;
  }
}

// called on fiber returning
static void fiber_exit() {
  // free
  append_fcbs(&this_f_env->exited_fcbs, this_f_env->running_fcb);
  this_f_env->running_fcb = NULL;

  _fiber_switch(this_f_env->sched_fcb->ctx, NULL);
}

// signal handler
static void signal_handler(int signum) {
  fprintf(stderr, "FAULT: fiber(%d) raised exception(%d). killed.\n",
          this_f_env->running_fcb->fid, signum);
  fiber_exit();
}

int fiber_init() {
  // reset global variables
  ava_fid = 0;
  if (this_f_env != NULL) {
    free(this_f_env);
    this_f_env = NULL;
  }

  // alloc & zero this_f_env
  this_f_env = (f_env *)malloc(sizeof(f_env));
  memset(this_f_env, 0, sizeof(f_env));

  // convert current exe unit to sched fiber
  fcb *sched_fcb = (fcb *)malloc(sizeof(fcb));
  memset(sched_fcb, 0, sizeof(fcb));

  sched_fcb->fid = ava_fid++;

  this_f_env->sched_fcb = sched_fcb;

  // setup signal handler
  static struct sigaction sa;
  sa.sa_handler = signal_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGSYS, &sa, NULL);

  return 0;
}

void fiber_sched() {
  while (this_f_env->running_fcb != NULL || this_f_env->ready_fcbs != NULL ||
         this_f_env->sleeping_fcbs != NULL || this_f_env->exited_fcbs != NULL) {
    // get current time
    unsigned int ts = get_mill_timestamp();

    // cleanup exited fibers
    fcb *exited_fcb = NULL;
    while ((exited_fcb = this_f_env->exited_fcbs) != NULL) {
      this_f_env->exited_fcbs = exited_fcb->next;
      free(exited_fcb->stack_base);
      free(exited_fcb);
    }

    // check sleeping fibers. move to ready fcbs if timeout
    // also check if there're some timeouts
    // if no fiber timeouts, this thread goes to sleep too for min sleeping time
    int has_timeout = 0;
    unsigned int min_sleeping_time = 0;

    fcb *sleeping_fcb = this_f_env->sleeping_fcbs;
    while (sleeping_fcb != NULL) {
      if (sleeping_fcb->sleep_to > ts) {
        // recored min sleeping time
        unsigned int time_left = sleeping_fcb->sleep_to - ts;
        if (min_sleeping_time == 0 || time_left < min_sleeping_time) {
          min_sleeping_time = time_left;
        }

        sleeping_fcb = sleeping_fcb->next;
        continue;
      }
      has_timeout++;

      if (sleeping_fcb->prev == NULL) {
        // head
        this_f_env->sleeping_fcbs = sleeping_fcb->next;
      } else {
        sleeping_fcb->prev->next = sleeping_fcb->next;
      }
      if (sleeping_fcb->next != NULL) {
        sleeping_fcb->next->prev = sleeping_fcb->prev;
      }

      // append to ready ones
      append_fcbs(&this_f_env->ready_fcbs, sleeping_fcb);

      sleeping_fcb = sleeping_fcb->next;
    }

    // if no timeout and min sleeping time > 0, thread sleeps
    if (this_f_env->running_fcb == NULL && this_f_env->ready_fcbs == NULL &&
        has_timeout == 0 && min_sleeping_time > 0) {
      struct timespec tm;
      tm.tv_sec = min_sleeping_time / 1000;
      tm.tv_nsec = (min_sleeping_time % 1000) * 1000000;
      nanosleep(&tm, NULL);
    }

    // prepare next fiber to be running one
    fcb *next_fcb = NULL;
    if ((next_fcb = this_f_env->ready_fcbs) != NULL) {
      // remove from ready fibers' head
      this_f_env->ready_fcbs = next_fcb->next;

      // append running fiber to ready ones
      if (this_f_env->running_fcb != NULL) {
        append_fcbs(&this_f_env->ready_fcbs, this_f_env->running_fcb);
      }

      // set next fiber running
      this_f_env->running_fcb = next_fcb;
    }

    // switch to running fiber
    if (this_f_env->running_fcb != NULL) {
      _fiber_switch(this_f_env->running_fcb->ctx, &this_f_env->sched_fcb->ctx);
    }
  }
}

int fiber_create(fentry entry, void *arg) {
  // alloc & zero fcb
  fcb *new_fcb = (fcb *)malloc(sizeof(fcb));
  memset(new_fcb, 0, sizeof(fcb));

  // alloc stack
  uint8_t *stack_base = (uint8_t *)malloc(FIBER_STACK_SIZE);
  new_fcb->stack_base = stack_base;

  // set ret addr of fiber entry to fiber_exit
  *(void **)(stack_base + FIBER_STACK_SIZE - sizeof(void *)) = fiber_exit;

  // makeup ctx
  new_fcb->ctx = (fe_ctx *)(stack_base + FIBER_STACK_SIZE - sizeof(void *) -
                            sizeof(fe_ctx));
  memset(new_fcb->ctx, 0, sizeof(fe_ctx));
  new_fcb->ctx->arg_1 = (intptr_t)arg;
  new_fcb->ctx->ret_addr = entry;

  // append to ready_fcbs
  append_fcbs(&this_f_env->ready_fcbs, new_fcb);

  // other stuff
  new_fcb->fid = ava_fid++;

  return new_fcb->fid;
}

void fiber_sleep(unsigned int n) {
  fcb *cur_fcb = this_f_env->running_fcb;
  this_f_env->running_fcb = NULL;

  cur_fcb->sleep_to = get_mill_timestamp() + n;

  // append to sleeping_fcbs
  append_fcbs(&this_f_env->sleeping_fcbs, cur_fcb);

  _fiber_switch(this_f_env->sched_fcb->ctx, &cur_fcb->ctx);
}

void fiber_yield() {
  _fiber_switch(this_f_env->sched_fcb->ctx, &this_f_env->running_fcb->ctx);
}