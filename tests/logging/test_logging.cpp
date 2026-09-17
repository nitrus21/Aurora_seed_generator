#include "aurora_log.h"
int main() {
  int evaluations = 0;
  AURORA_DIAG("AURORA_LOG_TEST_MARKER %d\n", ++evaluations);
  return evaluations == AURORA_DEBUG ? 0 : 1;
}
