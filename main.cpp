#include "hm.hpp"

#include <iostream>
#include <vector>

using namespace hm;

int main() {
  std::vector<std::tuple<type_env_t, std::shared_ptr<expr>>> tests;

  tests.emplace_back(
      type_env_t{},
      expr::mkLet("id", expr::mkAbs("x", expr::mkVar("x")), expr::mkVar("id")));

  tests.emplace_back(
      type_env_t{},
      expr::mkLet("id", expr::mkAbs("x", expr::mkVar("x")),
                  expr::mkApp(expr::mkVar("id"), expr::mkVar("id"))));

  tests.emplace_back(
      type_env_t{},
      expr::mkLet(
          "id",
          expr::mkAbs("x",

                      expr::mkLet("y", expr::mkVar("x"), expr::mkVar("y"))),
          expr::mkApp(expr::mkVar("id"), expr::mkVar("id"))));

  tests.emplace_back(
      type_env_t{},
      expr::mkLet(
          "id",
          expr::mkAbs("x",

                      expr::mkLet("y", expr::mkVar("x"), expr::mkVar("y"))),
          expr::mkApp(expr::mkApp(expr::mkVar("id"), expr::mkVar("id")),
                      expr::mkPrim(prim::mkString("Hello")))));

  tests.emplace_back(
      type_env_t{}, expr::mkLet("id",
                                expr::mkAbs("x", expr::mkApp(expr::mkVar("x"),
                                                             expr::mkVar("x"))),
                                expr::mkVar("id")));

  tests.emplace_back(
      type_env_t{},
      expr::mkAbs(
          "m",
          expr::mkLet("y", expr::mkVar("m"),
                      expr::mkLet("x",
                                  expr::mkApp(expr::mkVar("y"),
                                              expr::mkPrim(prim::mkTrue())),
                                  expr::mkVar("x")))));

  tests.emplace_back(
      type_env_t{
          {"S", std::tuple{std::vector<tv_t>{},
                           type::mkStruct(struct_t{
                               {hm::id_t{"a"}, type::mkPrim(prim::Int)}})}}},

      expr::mkStruct(
          struct_v{.type_ = "S",
                   .fields_ = std::vector{std::tuple{
                       hm::id_t{"a"}, expr::mkPrim(prim::mkInt(1))}}}));

  tests.emplace_back(
      type_env_t{{"S", std::tuple{std::vector<tv_t>{"T"},
                                  type::mkStruct(struct_t{
                                      {hm::id_t{"a"}, type::mkVar("T")}})}}},

      expr::mkStruct(
          struct_v{.type_ = "S",
                   .fields_ = std::vector{std::tuple{
                       hm::id_t{"a"}, expr::mkPrim(prim::mkInt(1))}}}));

  tests.emplace_back(
      type_env_t{{"S", std::tuple{std::vector<tv_t>{"T"},
                                  type::mkStruct(struct_t{
                                      {hm::id_t{"a"}, type::mkVar("T")},
                                      {hm::id_t{"b"},
                                       type::mkTuple({type::mkVar("T"),
                                                      type::mkVar("T")})}})}}},

      expr::mkStruct(struct_v{
          .type_ = "S",
          .fields_ =
              std::vector{std::tuple{
                              hm::id_t{"a"},
                              expr::mkPrim(prim::mkInt(1)),
                          },
                          std::tuple{
                              hm::id_t{"b"},
                              expr::mkTuple({expr::mkPrim(prim::mkInt(1)),
                                             expr::mkPrim(prim::mkInt(2))}),
                          }}}));

  tests.emplace_back(
      type_env_t{{"S", std::tuple{std::vector<tv_t>{"T"},
                                  type::mkStruct(struct_t{
                                      {hm::id_t{"a"}, type::mkVar("T")},
                                      {hm::id_t{"b"},
                                       type::mkTuple({type::mkVar("T"),
                                                      type::mkVar("T")})}})}}},

      expr::mkStruct(struct_v{
          .type_ = "S",
          .fields_ =
              std::vector{std::tuple{
                              hm::id_t{"a"},
                              expr::mkPrim(prim::mkInt(1)),
                          },
                          std::tuple{
                              hm::id_t{"b"},
                              expr::mkTuple({expr::mkPrim(prim::mkInt(1)),
                                             expr::mkPrim(prim::mkFalse())}),
                          }}}));

  tests.emplace_back(type_env_t{}, expr::mkCond(expr::mkPrim(prim::mkTrue()),
                                                expr::mkPrim(prim::mkInt(1)),
                                                expr::mkPrim(prim::mkInt(2))));

  tests.emplace_back(
      type_env_t{},
      expr::mkAbs(
          "x", expr::mkAbs("y", expr::mkCond(expr::mkVar("x"), expr::mkVar("y"),
                                             expr::mkPrim(prim::mkInt(2))))));

  tests.emplace_back(
      type_env_t{},
      expr::mkApp(

          expr::mkAbs(
              "x",
              expr::mkAbs("y", expr::mkCond(expr::mkVar("x"), expr::mkVar("y"),
                                            expr::mkPrim(prim::mkInt(2))))),
          expr::mkPrim(prim::mkInt(1))));

  for (const auto &[env, test] : tests) {
    std::cout << "=========================================================="
              << std::endl;
    if (!env.empty()) {
      std::cout << "Env: " << std::endl;
      for (const auto &[k, v] : env) {
        std::cout << k << " : " << show_scheme(v) << std::endl;
      }
    }
    std::cout << "Test: " << test->to_string() << std::endl;
    auto res = run_ti(test, env);

    if (!res.has_value()) {
      std::cout << "Failed: " << res.error() << std::endl;
      continue;
    }

    auto [s, t] = res.value();

    std::cout << "Res: " << t->to_string() << std::endl;
    std::cout << "Subst: " << std::endl;
    for (const auto &[k, v] : s) {
      std::cout << k << " => " << v->to_string() << std::endl;
    }
  }

  return 0;
}
