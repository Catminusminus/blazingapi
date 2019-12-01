#include "httplib.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include <string>
#include <functional>
#include <vector>
#include <sstream>
#include <utility>

namespace blazingapi {
  using QueryParams = httplib::Params;
  using PathParams = std::vector<std::string>;

  namespace detail {
    template <typename T> struct function_traits;

    template <typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>> {
      static const auto nargs = sizeof...(Args);
      using args = std::tuple<Args...>;
      template <std::size_t I> struct arg {
        using type = std::tuple_element_t<I, std::tuple<Args...>>;
      };
      template <std::size_t I>
      using arg_t = typename arg<I>::type;
    };

    template <class Function> 
    constexpr auto make_function(Function&& func) {
      std::function func_(std::forward<Function>(func));
      return func_;
    }

    template <class T>
    auto to_string(const T& x) {
      if constexpr(std::is_base_of_v<nlohmann::json, T>) {
        return x.dump();
      } else {
        return x;
      }
    };

    template <class F, class T, T... Seq>
    auto make_applied_tuple(F func, std::integer_sequence<T, Seq...>) {
      return std::make_tuple(func(Seq)...);
    }
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
          if constexpr(detail::function_traits<decltype(detail::make_function(f))>::nargs == 0) {
            res.set_content(detail::to_string(std::invoke(f)), "application/json");
          } else if constexpr(detail::function_traits<decltype(detail::make_function(f))>::nargs == 1) {
            using arg = typename detail::function_traits<decltype(detail::make_function(f))>::template arg_t<0>;
            if constexpr(std::is_same_v<std::decay_t<arg>, PathParams>) {
              PathParams path_params_;
              for (std::size_t i = 1, n = req.matches.size(); i < n; ++i)
                path_params_.emplace_back(req.matches.str(i));
              res.set_content(detail::to_string(std::invoke(f, path_params_)), "application/json");
            } else if constexpr(std::is_same_v<std::decay_t<arg>, QueryParams>) {
              res.set_content([&f, &req]{ return detail::to_string(std::invoke(f, req.params)); }(), "application/json");
            } else {
              const auto path_param = req.matches.str(1);
              auto path_stream = std::istringstream(path_param);
              std::decay_t<arg> path_arg;
              path_stream >> path_arg;
              res.set_content([&f, &path_arg]{ return detail::to_string(std::invoke(f, path_arg)); }(), "application/json");
            }
          } else if constexpr(std::is_same_v<std::decay_t<typename detail::function_traits<decltype(detail::make_function(f))>::template arg_t<detail::function_traits<decltype(detail::make_function(f))>::nargs - 1>>, QueryParams>) {
            using arg = typename detail::function_traits<decltype(detail::make_function(f))>::template arg_t<0>;
            if constexpr(std::is_same_v<std::decay_t<arg>, PathParams>) {
              PathParams path_params;
              for (std::size_t i = 1, n = req.matches.size(); i < n; ++i)
                path_params.emplace_back(req.matches.str(i));
              res.set_content(detail::to_string(std::invoke(f, path_params, req.params)), "application/json");
            } else {
              const auto func = [&](std::size_t i){
                const auto path_param = req.matches.str(i + 1);
                auto path_stream = std::istringstream(path_param);
                std::decay_t<arg> path_arg;
                path_stream >> path_arg;
                return path_arg;
              };
              constexpr auto number_of_path_params = detail::function_traits<decltype(detail::make_function(f))>::nargs - 1;
              auto applied_tuple = detail::make_applied_tuple(func, std::make_index_sequence<number_of_path_params>());
              res.set_content(detail::to_string(std::apply(f, std::tuple_cat(applied_tuple, std::make_tuple(req.params)))), "application/json");
            }
          } else {
            using arg = typename detail::function_traits<decltype(detail::make_function(f))>::template arg_t<0>;
            const auto func = [&](std::size_t i){
              const auto path_param = req.matches.str(i + 1);
              auto path_stream = std::istringstream(path_param);
              std::decay_t<arg> path_arg;
              path_stream >> path_arg;
              return path_arg;
            };
            constexpr auto number_of_path_params = detail::function_traits<decltype(detail::make_function(f))>::nargs;
            auto applied_tuple = detail::make_applied_tuple(func, std::make_index_sequence<number_of_path_params>());
            res.set_content(detail::to_string(std::apply(f, applied_tuple)), "application/json");
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
