#include "blazingapi.hpp"
#include <sstream>

int main() {
  using namespace blazingapi;

  auto server = BlazingAPI();
  server.Get("/hi") = [] { return R"({"hello":"world"})"; };
  server.Get(R"(/yo/(\d+))") = [](const blazingapi::PathParams &path_params) {
    const auto number1_str = path_params.at(0);
    auto number1_stream = std::istringstream(number1_str);
    int number1;
    number1_stream >> number1;
    using json = nlohmann::json;
    json j;
    j["number1"] = number1;
    return j;
  };
  server.Get(R"(/ho/(\d+))") = [](int path_param) {
    using json = nlohmann::json;
    json j;
    j["number"] = path_param;
    return j;
  };
  server.Get(R"(/hoho/(\d+)/(\d+))") = [](int path_param_int,
                                          float path_param_float) {
    using json = nlohmann::json;
    json j;
    j["int_num"] = path_param_int;
    j["float_num"] = path_param_float;
    return j;
  };
  server.Get(R"(/yoho/(\d+))") = [](int path_param,
                                    const QueryParams &query_params) {
    using json = nlohmann::json;
    json j;
    j["p"] = (query_params.find("p")->second);
    j["number"] = path_param;
    return j;
  };
  server.Get(R"(/numbers/(\d+)/numbers/(\d+))") =
      [](const PathParams &path_params, const QueryParams &query_params) {
        const auto number1 = path_params.at(0);
        using json = nlohmann::json;
        json j;
        j["p"] = (query_params.find("p")->second);
        j["number1"] = number1;
        return j;
      };
  server.Delete("/delete") = [] { return R"({"msg": "ok"})"; };
  server.run(1234);
}

// curl -v "http://localhost:1234/hi" => {"hello":"world"}
// curl -v "http://localhost:1234/yo/5" => {"number1":5}
// curl -v "http://localhost:1234/ho/5" => {"number":5}
// curl -v "http://localhost:1234/hoho/5/10" => {"float_num":10.0,"int_num":5}
// curl -v "http://localhost:1234/yoho/5?p=aaaa" => {"number":5,"p":"aaaa"}
// curl -v "http://localhost:1234/numbers/5/numbers/6?q=aaaa&p=bbbb&r=cccc" => {"number1":"5","p":"bbbb"}
// curl -v "http://localhost:1234/nonexist" => {"detail":"Not Found"} 
// curl -X DELETE -v "http://localhost:1234/delete" => {"msg": "ok"}
