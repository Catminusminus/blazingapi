#include "blazingapi.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace blazingapi;
  using namespace boost::ut;
  "get(/hi) will be ok"_test = [] {
    auto server = BlazingAPI();
    server.Get("/hi") = [] { return R"({"hello":"world"})"; };
    auto client = TestClient(server, 8080);
    auto response = client.Get("/hi");
    expect(response->status == 200_i);
  };
}
