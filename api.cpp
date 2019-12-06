#include "blazingapi.hpp"
#include <sstream>

namespace blazingapi::ns {
struct Item : public blazingapi::RequestBody {
  int id;
  double num;
  std::string description;
};
} // namespace blazingapi::ns

REFL_AUTO(type(blazingapi::ns::Item), field(id), field(num), field(description))

int main() {
  using namespace blazingapi;
  using json = nlohmann::json;

  auto server = BlazingAPI();
  server.Get("/hi") = [] { return R"({"hello":"world"})"; };
  server.Get(R"(/yo/(\d+))") = [](const blazingapi::PathParams &path_params) {
    const auto number1_str = path_params.at(0);
    auto number1_stream = std::istringstream(number1_str);
    int number1;
    number1_stream >> number1;
    json j;
    j["number1"] = number1;
    return j;
  };
  server.Get(R"(/ho/(\d+))") = [](int path_param) {
    json j;
    j["number"] = path_param;
    return j;
  };
  server.Get(R"(/hoho/(\d+)/(\d+))") = [](int path_param_int,
                                          float path_param_float) {
    json j;
    j["int_num"] = path_param_int;
    j["float_num"] = path_param_float;
    return j;
  };
  server.Get(R"(/yoho/(\d+))") = [](int path_param,
                                    const QueryParams &query_params) {
    json j;
    j["p"] = (query_params.find("p")->second);
    j["number"] = path_param;
    return j;
  };
  server.Get(R"(/numbers/(\d+)/numbers/(\d+))") =
      [](const PathParams &path_params, const QueryParams &query_params) {
        const auto number1 = path_params.at(0);
        json j;
        j["p"] = (query_params.find("p")->second);
        j["number1"] = number1;
        return j;
      };
  server.Delete("/delete") = [] { return R"({"msg": "ok"})"; };
  server.Post("/post") = [](ns::Item item) {
    json j;
    j["id"] = item.id;
    return j;
  };
  server.run(1234);
}

// curl -v "http://localhost:1234/hi" => {"hello":"world"}
// curl -v "http://localhost:1234/yo/5" => {"number1":5}
// curl -v "http://localhost:1234/ho/5" => {"number":5}
// curl -v "http://localhost:1234/hoho/5/10" => {"float_num":10.0,"int_num":5}
// curl -v "http://localhost:1234/yoho/5?p=aaaa" => {"number":5,"p":"aaaa"}
// curl -v "http://localhost:1234/numbers/5/numbers/6?q=aaaa&p=bbbb&r=cccc" =>
// {"number1":"5","p":"bbbb"} curl -v "http://localhost:1234/nonexist" =>
// {"detail":"Not Found"} curl -X DELETE -v "http://localhost:1234/delete" =>
// {"msg": "ok"}
