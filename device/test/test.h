#include <stdio.h>

#define TEST(Suite, Test) \
  void TEST_##Suite##_##Test(); \
  void test_register_##Suite##_##Test() __attribute__((constructor)); \
  void test_register_##Suite##_##Test() { \
    static test_t test_def = { #Suite, #Test, &TEST_##Suite##_##Test }; \
    if (!g_first_test) { \
      g_first_test = &test_def; \
      g_last_test = &test_def; \
    } else if (g_last_test) { \
      g_last_test->next = &test_def; \
      g_last_test = &test_def; \
    } \
  } \
  void TEST_##Suite##_##Test()

const char* _test_current_suite_name();
const char* _test_current_test_name();

void _test_expectation_failed();

#define EXPECT(condition) \
  if (condition); else { _test_expectation_failed(); fprintf(stderr, "[ \x1b[31mFAILED\x1b[0m ] %s.%s at " __FILE__ ":%d: " #condition "\n", _test_current_suite_name(), _test_current_test_name(), __LINE__); }

struct test_t;

typedef struct test_t {
  const char* suite_name;
  const char* test_name;
  void (*fn)();
  struct test_t* next;
} test_t;

extern test_t* g_first_test;
extern test_t* g_last_test;

int test_all();
