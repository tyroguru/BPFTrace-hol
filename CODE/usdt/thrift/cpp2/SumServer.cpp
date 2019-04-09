#include <folly/init/Init.h>

#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "bpftrace-hol/cpp2/SumHandler.h"
#include "common/services/cpp/ServiceFramework.h"

using namespace sum;

DEFINE_int32(port, 23456, "Port for Sum Service");

int main(int argc, char **argv) {
  folly::init(&argc, &argv);
  facebook::services::ServiceFramework service("Sum Service");
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<SumHandler>();
  server->setInterface(handler);
  server->setPort(FLAGS_port);

  service.addThriftService(server, handler.get(), FLAGS_port);
  service.go();

  return 0;
}
