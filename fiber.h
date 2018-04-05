#ifndef __FIBER_H
#define __FIBER_H

#include <stddef.h>
#include <stdint.h>

// config

#define FIBER_STACK_SIZE 0x500000

// types

// fiber execution context
typedef struct {
  intptr_t ret_v;
  intptr_t arg_1;

  intptr_t rbx;
  intptr_t rbp;
  intptr_t r12;
  intptr_t r13;
  intptr_t r14;
  intptr_t r15;
  void *ret_addr;
} fe_ctx;

// fiber control block
typedef struct _fcb {
  // fiber info
  int fid;
  void *stack_base;
  fe_ctx *ctx;
  struct _fcb *prev;
  struct _fcb *next;

  // sleep related
  unsigned int sleep_to;
} fcb;

// fiber environment
typedef struct {
  fcb *sched_fcb;
  fcb *running_fcb;
  fcb *ready_fcbs;
  fcb *sleeping_fcbs;
  fcb *freed_fcbs;
} f_env;

// fiber entry
typedef void (*fentry)(void *arg);

// methods

// initialize the global fiber environment &
// convert current execution unit to a fiber on which the scheduler will be run
// returns 0 on success or <0 on error
int fiber_init();

// start scheduler
// returns when no fiber on track
void fiber_sched();

// create a new fiber, returning fid or <0 on error
int fiber_create(fentry entry, void *arg);

// put current fiber to sleep for n ms
void fiber_sleep(unsigned int n);

// yield
void fiber_yield();

#endif // __FIBER_H