#include "httplib.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include <string>
#include <functional>
#include <vector>

namespace blazingapi {
  using QueryParams = httplib::Params;
  using PathParams = std::vector<std::string>;

  namespace detail {
    template <typename T> struct function_traits;

    template <typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>> {
      static const auto nargs = sizeof...(Args);
      using args = std::tuple<Args...>;
    };

    template <class Function> 
    constexpr auto make_function(Function&& func) {
      std::function func_(std::forward<Function>(func));
      return func_;
    }

    constexpr auto get_number_of_args = [](auto&& func) {
      return function_traits<decltype(make_function(std::forward<decltype(func)>(func)))>::nargs;
    };

    template <class T>
    auto to_string(const T& x) {
      if constexpr(std::is_base_of_v<nlohmann::json, T>) {
        return x.dump();
      } else {
        return x;
      }
    };
  }
  class BlazingAPI{
    httplib::Server internal_server;
  public:
    using this_type = BlazingAPI;
  private:
    template <class Method>
    class BlazingAPIInternal{
      httplib::Server& sever;
      Method method;
      const std::string url;
    public:
      BlazingAPIInternal(httplib::Server& sever, Method method, const std::string_view url): sever(sever), method(method), url(url) {}
      template <class F>
      auto operator=(F&& f) {
        auto return_json = [&f](const httplib::Request &req, httplib::Response &res) {
          //bad
          if constexpr(detail::function_traits<decltype(detail::make_function(f))>::nargs == 0) {
            res.set_content(detail::to_string(std::invoke(f)), "application/json");
          } else {
            PathParams path_params;
            for (std::size_t i = 1, n = req.matches.size(); i < n; ++i)
              path_params.emplace_back(req.matches.str(i));

            res.set_content(detail::to_string(std::invoke(f, path_params, req.params)), "application/json");
          }
        };
        std::invoke(method, sever, url.c_str(), std::move(return_json));
      }
    };
  public:
    auto Get(const std::string_view url) {
      internal_server.set_error_handler([](const httplib::Request & /*req*/, httplib::Response &res) {
        if (res.status == 404) {
          res.set_content(R"({"detail":"Not Found"})", "application/json");
        } else {
          res.set_content(R"({"detail":"Something Wrong"})", "application/json");
        }
      });
      internal_server.set_logger([](const auto& req, const auto& res) {
        spdlog::info("Request: {0} {1} {2}", req.method, req.path, res.status);
        for (auto [key, value] : req.params)
          spdlog::info("Request params: {0} {1}", key, value);
        for (std::size_t i = 0, n = req.matches.size(); i < n; ++i)
          spdlog::info("Request matches: {}", req.matches.str(i));
        spdlog::info("number: {}", req.matches.size() - 1 + req.params.size());
      });
      return BlazingAPIInternal(internal_server, &httplib::Server::Get, url);
    }
    auto Post(const std::string& url) {}
    auto Put(const std::string& url) {}
    auto Delete(const std::string& url) {}
    auto run(unsigned int port_number) {
      internal_server.listen("localhost", port_number);
    }
  };
}
