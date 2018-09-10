#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test.h"

test_t* g_first_test;
test_t* g_last_test;

static test_t* current_test;
static int failed_expectations;

const char* _test_current_suite_name() {
  return current_test->suite_name;
}
const char* _test_current_test_name() {
  return current_test->test_name;
}

void _test_expectation_failed() {
  failed_expectations++;
}

int test_all() {
  int total_failed_expectations = 0;
  for (current_test = g_first_test; current_test; current_test = current_test->next) {
    pid_t child_pid = fork();
    if (child_pid) {
      int status;
      EXPECT(wait(&status) == child_pid);
      total_failed_expectations += WEXITSTATUS(status);
    } else {
      current_test->fn();
      exit(failed_expectations);
    }
  }
  if (!total_failed_expectations)
    fprintf(stderr, "A-OK\n");
  return total_failed_expectations;
}
