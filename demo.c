#include <stdio.h>

#include "fiber.h"

void task1(void *arg) {
  for (int i = 0; i < 4; i++) {
    printf("s task1(%d)\n", i);
    fiber_sleep(500);
  }
}

void task2(void *arg) {
  for (int i = 0; i < 2; i++) {
    printf("s task2(%d)\n", i);
    fiber_sleep(2000);
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
  }
}

void task5(void *arg) {
  for (int i = 0; i < 5; i++) {
    printf("y task5(%d)\n", i);
    fiber_yield();
    if (i == 1) {
      i = i / (int)arg;
    }
  }
}

int main() {
  fiber_init();

  int f1 = fiber_create(task1, NULL);
  printf("created task1 no. %d\n", f1);

  int f2 = fiber_create(task2, NULL);
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