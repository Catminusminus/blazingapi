#include "blazingapi.hpp"
#include <iostream>
#include <sstream>

int main() {
  using namespace blazingapi;
  auto server = BlazingAPI();
  server.Get("/hi") = [] { return R"({"hello":"world"})"; };
  auto client = TestClient(server);
  client.run(8080);
  auto response = client.Get("/hi");
  assert(response->status == 200);
}
