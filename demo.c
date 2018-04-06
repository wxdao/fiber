#include <stdio.h>

#include "fiber.h"

void task1(void *arg) {
  for (int i = 0; i < 4; i++) {
    printf("s task1(%d)\n", i);
    fiber_sleep((int)arg);
  }
}

void task2(void *arg) {
  for (int i = 0; i < 2; i++) {
    printf("s task2(%d)\n", i);
    fiber_sleep((int)arg);
  }
}

void task3(void *arg) {
  for (int i = 0; i < 5; i++) {
    printf("y task3(%d)\n", i);
    fiber_yield();
  }
}

void task4(void *arg) {
  for (int i = 0; i < 5; i++) {
    printf("y task4(%d)\n", i);
    fiber_yield();
    if (i == 4) {
      // trigger divide by zero exception
      i /= (int)arg;
    }
  }
}

void task5(void *arg) {
  for (int i = 0; i < 5; i++) {
    printf("y task5(%d)\n", i);
    fiber_yield();
    if (i == 1) {
      // trigger NULL memory access exception
      *(int *)arg = 1;
    }
  }
}

int main() {
  fiber_init();

  int f1 = fiber_create(task1, (void *)500);
  printf("created task1 no. %d\n", f1);

  int f2 = fiber_create(task2, (void *)2000);
  printf("created task2 no. %d\n", f2);

  int f3 = fiber_create(task3, NULL);
  printf("created task3 no. %d\n", f3);

  int f4 = fiber_create(task4, NULL);
  printf("created task4 no. %d\n", f4);

  int f5 = fiber_create(task5, NULL);
  printf("created task4 no. %d\n", f5);

  printf("starting sched\n");

  fiber_sched();

  printf("all tasks existed\n");
}