#include "bpftrace-hol/cpp2/SumHandler.h"
#include "bpftrace-hol/if/gen-cpp2/SumService.h"
#include "thrift/lib/cpp/TApplicationException.h"
#include <folly/tracing/StaticTracepoint.h>

using namespace sum;

SumHandler::SumHandler() :
  FacebookBase2("Sum Service") {
}

facebook::fb303::cpp2::fb_status SumHandler::getStatus() {
  return facebook::fb303::cpp2::fb_status::ALIVE;
}

int32_t SumHandler::sum(int32_t num1, int32_t num2) {
  FOLLY_SDT(testthrift, sum, num1, num2);
  return num1 + num2;
}
