#pragma once

#include "bpftrace-hol/if/gen-cpp2/SumService.h"
#include "common/fb303/cpp/FacebookBase2.h"

namespace sum {
  class SumHandler : public sum::SumServiceSvIf,
                   public facebook::fb303::FacebookBase2 {
    public:
      SumHandler();
      facebook::fb303::cpp2::fb_status getStatus();
      virtual int32_t sum(int32_t num1, int32_t num2);
  };
} //sum
