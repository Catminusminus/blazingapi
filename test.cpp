#include "blazingapi.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace blazingapi;
  using namespace boost::ut;
  "get(/hi) test"_test = [] {
    auto server = BlazingAPI();
    server.Get("/hi") = [] { return R"({"hello":"world"})"; };
    auto client = TestClient(server, 8080);
    "ok"_test = [&] {
      auto response = client.Get("/hi");
      expect(response->status == 200_i);
    };
    // this test will be failed
    "ng"_test = [&] {
      auto response = client.Get("/yo");
      expect(response->status == 400_i);
    };
  };
}
