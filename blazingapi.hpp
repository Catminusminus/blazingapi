#include "httplib.h"
#include "nlohmann/json.hpp"
#include "refl.hpp"
#include "spdlog/spdlog.h"
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace blazingapi::ns {
using json = nlohmann::json;
template <class T> auto from_json(const json &j, T &p) {
  refl::util::for_each(refl::reflect(p).members, [&](auto member) {
    j.at(member.name.c_str()).get_to(member(p));
  });
}
} // namespace blazingapi::ns

namespace blazingapi {
using QueryParams = httplib::Params;
using PathParams = std::vector<std::string>;

struct RequestBody {};
} // namespace blazingapi

namespace blazingapi {

namespace detail {
template <typename T> struct function_traits;

template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
  static const auto nargs = sizeof...(Args);
  using args = std::tuple<Args...>;
  template <std::size_t I> struct arg {
    using type = std::tuple_element_t<I, std::tuple<Args...>>;
  };
  using decayed_args = std::tuple<std::decay_t<Args>...>;
  template <std::size_t I> using arg_t = typename arg<I>::type;
};

template <class Function> constexpr auto make_function(Function &&func) {
  std::function func_(std::forward<Function>(func));
  return func_;
}

template <class T> auto to_string(const T &x) {
  if constexpr (std::is_base_of_v<nlohmann::json, T>) {
    return x.dump();
  } else {
    return x;
  }
};

template <class F, class T, T... Seq>
auto make_applied_tuple(F func, std::integer_sequence<T, Seq...>) {
  return std::make_tuple(func(Seq)...);
}

template <class F, class T, T... Seq>
auto make_applied_tuple_with_helper(const httplib::Request &req, F,
                                    std::integer_sequence<T, Seq...>) {
  return std::make_tuple(F::template get_transformed_path_arg<Seq>(req)...);
}

template <class F> constexpr auto set_content(httplib::Response &res, F &&f) {
  res.set_content(detail::to_string(std::invoke(std::forward<F>(f))),
                  "application/json");
}

template <class... Args> struct ContentMaker {};

template <> struct ContentMaker<std::tuple<>> {
  template <class F>
  static auto set_content(const httplib::Request &req, httplib::Response &res,
                          F &&f) {
    res.set_content(detail::to_string(std::invoke(std::forward<F>(f))),
                    "application/json");
  }
};

template <> struct ContentMaker<std::tuple<PathParams>> {
  template <class F>
  static auto set_content(const httplib::Request &req, httplib::Response &res,
                          F &&f) {
    PathParams path_params;
    for (std::size_t i = 1, n = req.matches.size(); i < n; ++i)
      path_params.emplace_back(req.matches.str(i));
    res.set_content(
        detail::to_string(std::invoke(std::forward<F>(f), path_params)),
        "application/json");
  }
};

template <> struct ContentMaker<std::tuple<QueryParams>> {
  template <class F>
  static auto set_content(const httplib::Request &req, httplib::Response &res,
                          F &&f) {
    res.set_content(
        detail::to_string(std::invoke(std::forward<F>(f), req.params)),
        "application/json");
  }
};

template <> struct ContentMaker<std::tuple<PathParams, QueryParams>> {
  template <class F>
  static auto set_content(const httplib::Request &req, httplib::Response &res,
                          F &&f) {
    PathParams path_params;
    for (std::size_t i = 1, n = req.matches.size(); i < n; ++i)
      path_params.emplace_back(req.matches.str(i));
    res.set_content(detail::to_string(std::invoke(std::forward<F>(f),
                                                  path_params, req.params)),
                    "application/json");
  }
};

template <class Tuple> struct ArgHelper {
  template <std::size_t I>
  static auto get_transformed_path_arg(const httplib::Request &req) {
    const auto path_param = req.matches.str(I + 1);
    auto path_stream = std::istringstream(path_param);
    using type = std::tuple_element_t<I, Tuple>;
    type path_arg;
    path_stream >> path_arg;
    return path_arg;
  }
};

template <class T> struct ContentMaker<std::tuple<T>> {
  template <class F>
  static auto set_content(const httplib::Request &req, httplib::Response &res,
                          F &&f) {
    if constexpr (std::is_base_of_v<RequestBody, T>) {
      auto body_j = nlohmann::json::parse(req.body);
      auto body = body_j.get<T>();
      res.set_content(detail::to_string(std::invoke(std::forward<F>(f), body)),
                      "application/json");
    } else {
      res.set_content(
          detail::to_string(std::invoke(
              std::forward<F>(f),
              ArgHelper<std::tuple<T>>::template get_transformed_path_arg<0>(
                  req))),
          "application/json");
    }
  }
};

template <class... Args> struct ContentMaker<std::tuple<Args...>> {
  template <class F>
  static auto set_content(const httplib::Request &req, httplib::Response &res,
                          F &&f) {
    constexpr auto nargs = sizeof...(Args);
    using last_type = std::tuple_element_t<nargs - 1, std::tuple<Args...>>;
    if constexpr (std::is_same_v<last_type, QueryParams>) {
      constexpr auto number_of_path_params =
          detail::function_traits<decltype(detail::make_function(f))>::nargs -
          1;
      auto applied_tuple = detail::make_applied_tuple_with_helper(
          req, ArgHelper<std::tuple<Args...>>(),
          std::make_index_sequence<number_of_path_params>());
      res.set_content(
          detail::to_string(std::apply(
              std::forward<F>(f),
              std::tuple_cat(applied_tuple, std::make_tuple(req.params)))),
          "application/json");
    } else {
      constexpr auto number_of_path_params_ =
          detail::function_traits<decltype(detail::make_function(f))>::nargs;
      auto applied_tuple_ = detail::make_applied_tuple_with_helper(
          req, ArgHelper<std::tuple<Args...>>(),
          std::make_index_sequence<number_of_path_params_>());
      res.set_content(
          detail::to_string(std::apply(std::forward<F>(f), applied_tuple_)),
          "application/json");
    }
  }
};
} // namespace detail
class BlazingAPI {
  httplib::Server internal_server;

public:
  using this_type = BlazingAPI;

private:
  template <class Method> class BlazingAPIInternal {
    httplib::Server &server;
    Method method;
    const std::string url;

  public:
    BlazingAPIInternal(httplib::Server &server, Method method,
                       const std::string_view url)
        : server(server), method(method), url(url) {}
    template <class F> auto operator=(F &&f) {
      auto return_json = [&f](const httplib::Request &req,
                              httplib::Response &res) {
        try {
          detail::ContentMaker<
              typename detail::function_traits<decltype(detail::make_function(
                  f))>::decayed_args>::set_content(req, res,
                                                   std::forward<F>(f));
        } catch (...) {
          res.status = 500;
        }
      };
      std::invoke(method, server, url.c_str(), std::move(return_json));
    }
  };

public:
  BlazingAPI() {
    internal_server.set_error_handler([](const httplib::Request & /*req*/,
                                         httplib::Response &res) {
      if (res.status == 404) {
        res.set_content(R"({"detail":"Not Found"})", "application/json");
      } else {
        res.set_content(R"({"detail":"Something Wrong"})", "application/json");
      }
    });
    internal_server.set_logger([](const auto &req, const auto &res) {
      spdlog::info("Request: {0} {1} {2}", req.method, req.path, res.status);
      for (auto [key, value] : req.params)
        spdlog::info("Request params: {0} {1}", key, value);
      for (std::size_t i = 0, n = req.matches.size(); i < n; ++i)
        spdlog::info("Request matches: {}", req.matches.str(i));
    });
  }
  decltype(auto) Get(const std::string &url) {
    return BlazingAPIInternal(internal_server, &httplib::Server::Get, url);
  }
  decltype(auto) Post(const std::string &url) {
    auto (httplib::Server::*pPost)(const char *, httplib::Server::Handler) =
        &httplib::Server::Post;
    return BlazingAPIInternal(internal_server, pPost, url);
  }
  decltype(auto) Put(const std::string &url) {}
  decltype(auto) Delete(const std::string &url) {
    return BlazingAPIInternal(internal_server, &httplib::Server::Delete, url);
  }
  auto run(unsigned int port_number) {
    internal_server.listen("localhost", port_number);
  }
};
} // namespace blazingapi
