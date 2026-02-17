#include <iostream>
#include "helloworld.pb.h"
#include "helloworld.grpc.pb.h"
#include "tutorial.pb.h"
#include "tutorial.grpc.pb.h"

int main() {
    helloworld::HelloRequest req;
    req.set_name("Rifat");

    std::cout << "Name from protobuf object: " << req.name() << '\n';
    std::cout << "gRPC/protobuf codegen is wired correctly.\n";
    return 0;
}
