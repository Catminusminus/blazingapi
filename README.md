# BlazingAPI
Inspired by [FastAPI](https://github.com/tiangolo/fastapi).

Stage: Experimental-Pre-Pre-Pre-Alpha

## Simple Example
```cpp
int main() {
  using namespace blazingapi;
  using json = nlohmann::json;

  auto server = BlazingAPI();
  
  // curl -v "http://localhost:1234/hi" => {"hello":"world"}
  server.Get("/hi") = [] { return R"({"hello":"world"})"; };
  
  // curl -v "http://localhost:1234/hoho/5/10" => {"float_num":10.0,"int_num":5}
  server.Get(R"(/hoho/(\d+)/(\d+))") = [](int path_param_int,
                                          float path_param_float) {
    json j;
    j["int_num"] = path_param_int;
    j["float_num"] = path_param_float;
    return j;
  };
  
  // curl -v "http://localhost:1234/yoho/5?p=aaaa" => {"number":5,"p":"aaaa"}
  server.Get(R"(/yoho/(\d+))") = [](int path_param,
                                    const QueryParams &query_params) {
    json j;
    j["p"] = (query_params.find("p")->second);
    j["number"] = path_param;
    return j;
  };
  
  server.run(1234);
  // curl -v "http://localhost:1234/nonexist" => {"detail":"Not Found"} 
}
```

## Post
```cpp
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

  // curl -X POST -v -d '{"id": 1, "num": 1.0, "description": "hi"}' "http://localhost:1234/post" => {"id": 1}
  server.Post("/post") = [](ns::Item item) {
    json j;
    j["id"] = item.id;
    return j;
  };

  server.run(1234);
}
```

## How to build the example
```
git clone https://github.com/yhirose/cpp-httplib.git
git clone https://github.com/nlohmann/json.git
git clone https://github.com/gabime/spdlog.git
git clone https://github.com/veselink1/refl-cpp.git

g++ -o api -std=c++17 -I./spdlog/include -I./cpp-httplib -I./json/single_include -I./refl-cpp -Wall -Wextra -pthread api.cpp -DCPPHTTPLIB_OPENSSL_SUPPORT -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lssl -lcrypto -DCPPHTTPLIB_ZLIB_SUPPORT -lz
```