#include "blazingapi.hpp"

int main() {
  using namespace blazingapi;

  auto server = BlazingAPI();
  server.Get("/hi") = [] {
    return R"({"hello":"world"})";
  };
  server.Get("/yo") = [] {
    return R"({"foo": "bar"})";
  };
  server.Get(R"(/numbers/(\d+)/numbers/(\d+))") = [] (const blazingapi::PathParams& path_params, const blazingapi::QueryParams& query_params){
    const auto number1 = path_params.at(0);
    using json = nlohmann::json;
    json j;
    j["p"] = (query_params.find("p")->second);
    j["number1"] = number1;
    return j;
  };
  server.run(1234);
}

// curl -v "http://localhost:1234/hi"
// curl -v "http://localhost:1234/numbers/5/numbers/6?q=1234&p=2345&r=3456"
