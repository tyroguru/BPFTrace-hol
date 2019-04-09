include "common/fb303/if/fb303.thrift"

namespace cpp2 sum
namespace py sum

service SumService extends fb303.FacebookService {
  i32 sum(1: i32 num1, 2: i32 num2);
}
