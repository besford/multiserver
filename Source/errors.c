#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "errors.h"

//Prints an error to standard error before terminating the thread.
void thr_error(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  pthread_exit(NULL);
}
void exception(const char* msg) { fprintf(stderr, "%s\n", msg); }

//Prints an error to standard error before terminating program.
void error(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(0);
}
