// Small, assertion-based regression cases for Algorithm W.
// Compile from the repository root with a C++23 toolchain supported by hm.hpp.
#include "../hm.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {
using namespace hm;

void check(bool ok, std::string_view message) {
  if (!ok) {
    throw std::runtime_error(std::string(message));
  }
}

bool is_prim(const std::shared_ptr<type> &t, prim::type p) {
  return t->kind_ == type::Prim && t->t.prim_ == p;
}

void substitution_composition_applies_right_to_left() {
  const subst_t s1{{"a", type::mkVar("b")}};
  const subst_t s2{{"b", type::mkPrim(prim::Int)}};
  const auto composed = compose_subst(s2, s1);
  check(is_prim(apply(composed, type::mkVar("a")), prim::Int),
        "composition must propagate transitive substitutions");
  check(is_prim(apply(composed, type::mkVar("b")), prim::Int),
        "composition must preserve the newer substitution");
}

void unification_propagates_constraints_from_codomain() {
  const auto a = type::mkVar("a");
  const auto b = type::mkVar("b");
  const auto result = unify(type::mkFn(a, a),
                            type::mkFn(b, type::mkPrim(prim::Int)));
  check(result.has_value(), "function types should unify");
  check(is_prim(apply(*result, a), prim::Int),
        "function unification did not propagate codomain constraint to a");
  check(is_prim(apply(*result, b), prim::Int),
        "function unification did not constrain b to int");
}

void occurs_check_rejects_infinite_types() {
  const auto a = type::mkVar("a");
  check(!unify(a, type::mkFn(a, type::mkPrim(prim::Int))).has_value(),
        "occurs check should reject a ~ a -> int");
}

void generalization_excludes_environment_variables() {
  const type_env_t env{{"x", scheme_t{std::vector<tv_t>{}, type::mkVar("a")}}};
  const auto sch = generalize(env, type::mkFn(type::mkVar("a"),
                                               type::mkVar("b")));
  check(std::get<0>(sch) == std::vector<tv_t>{"b"},
        "generalization must not quantify a variable free in Gamma");
}

void instantiation_uses_fresh_variables() {
  ti_state state;
  const scheme_t identity{{"a"},
                          type::mkFn(type::mkVar("a"), type::mkVar("a"))};
  const auto first = state.instantiate(identity);
  const auto second = state.instantiate(identity);
  check(first->kind_ == type::Fn && second->kind_ == type::Fn,
        "identity instances should remain function types");
  const auto [farg, fret] = first->t.fn_;
  const auto [sarg, sret] = second->t.fn_;
  check(farg->kind_ == type::Var && fret->kind_ == type::Var &&
            sarg->kind_ == type::Var && sret->kind_ == type::Var,
        "fresh instances should contain type variables");
  check(farg->t.var_ == fret->t.var_ && sarg->t.var_ == sret->t.var_,
        "each instance should preserve sharing inside the type");
  check(farg->t.var_ != sarg->t.var_,
        "different instantiations should use distinct fresh variables");
}

void scheme_substitution_respects_bound_variables() {
  const scheme_t scheme{{"a"},
                        type::mkFn(type::mkVar("a"), type::mkVar("b"))};
  const subst_t subst{{"a", type::mkPrim(prim::Int)},
                      {"b", type::mkPrim(prim::Bool)}};
  const auto updated = apply(subst, scheme);
  const auto body = std::get<1>(updated);
  check(std::get<0>(updated) == std::vector<tv_t>{"a"},
        "substitution should preserve quantifiers");
  check(body->kind_ == type::Fn, "scheme body should remain a function");
  const auto [arg, ret] = body->t.fn_;
  check(arg->kind_ == type::Var && arg->t.var_ == "a",
        "substitution must not replace a quantified variable");
  check(is_prim(ret, prim::Bool),
        "substitution should replace unbound variables in a scheme");
}

void let_polymorphism_instantiates_on_each_use() {
  const auto expr_ = expr::mkLet(
      "id", expr::mkAbs("x", expr::mkVar("x")),
      expr::mkTuple({expr::mkApp(expr::mkVar("id"),
                                expr::mkPrim(prim::mkInt(1))),
                     expr::mkApp(expr::mkVar("id"),
                                expr::mkPrim(prim::mkTrue()))}));
  const auto result = run_ti(expr_, {});
  check(result.has_value(), "let-polymorphism should accept two uses of id");
  const auto inferred = std::get<1>(*result);
  check(inferred->kind_ == type::Tuple && inferred->t.tuple_.size() == 2,
        "the result should be a pair");
  check(is_prim(inferred->t.tuple_[0], prim::Int) &&
            is_prim(inferred->t.tuple_[1], prim::Bool),
        "the expected result is (int, bool)");
}

void let_propagates_constraints_into_enclosing_lambda() {
  // \f -> let y = f 1 in y true
  // Principal type: (int -> bool -> a) -> a.
  const auto expr_ = expr::mkAbs(
      "f", expr::mkLet(
               "y", expr::mkApp(expr::mkVar("f"),
                                expr::mkPrim(prim::mkInt(1))),
               expr::mkApp(expr::mkVar("y"),
                           expr::mkPrim(prim::mkTrue()))));
  const auto result = run_ti(expr_, {});
  check(result.has_value(), "the expression should be typable");
  const auto inferred = std::get<1>(*result);
  check(inferred->kind_ == type::Fn, "result should be a function");
  const auto [f_type, result_type] = inferred->t.fn_;
  check(f_type->kind_ == type::Fn, "f should be a function");
  const auto [arg1, intermediate] = f_type->t.fn_;
  check(is_prim(arg1, prim::Int), "f should accept int first");
  check(intermediate->kind_ == type::Fn,
        "the result of f 1 must itself accept bool");
  const auto [arg2, final] = intermediate->t.fn_;
  check(is_prim(arg2, prim::Bool), "the second argument must be bool");
  check(final->kind_ == type::Var && result_type->kind_ == type::Var &&
            final->t.var_ == result_type->t.var_,
        "the function result variables must be identical");
}

void inference_rejects_self_application() {
  const auto omega_part = expr::mkAbs(
      "x", expr::mkApp(expr::mkVar("x"), expr::mkVar("x")));
  check(!run_ti(omega_part, {}).has_value(),
        "untyped self-application must fail the occurs check");
}

void inference_rejects_mismatched_branches() {
  const auto expr_ = expr::mkCond(expr::mkPrim(prim::mkTrue()),
                                  expr::mkPrim(prim::mkInt(1)),
                                  expr::mkPrim(prim::mkFalse()));
  check(!run_ti(expr_, {}).has_value(),
        "conditional branches must agree on a type");
}

struct test_case {
  std::string_view name;
  void (*run)();
};
} // namespace

int main() {
  const std::vector<test_case> tests{
      {"substitution composition", substitution_composition_applies_right_to_left},
      {"function unification composition", unification_propagates_constraints_from_codomain},
      {"occurs check", occurs_check_rejects_infinite_types},
      {"environment-aware generalization", generalization_excludes_environment_variables},
      {"fresh instantiation", instantiation_uses_fresh_variables},
      {"bound-variable substitution", scheme_substitution_respects_bound_variables},
      {"let-polymorphism", let_polymorphism_instantiates_on_each_use},
      {"let substitution propagation", let_propagates_constraints_into_enclosing_lambda},
      {"reject self-application", inference_rejects_self_application},
      {"reject mismatched branches", inference_rejects_mismatched_branches},
  };

  std::size_t failures = 0;
  for (const auto &test : tests) {
    try {
      test.run();
      std::cout << "PASS: " << test.name << '\n';
    } catch (const std::exception &e) {
      ++failures;
      std::cerr << "FAIL: " << test.name << ": " << e.what() << '\n';
    }
  }
  std::cout << (tests.size() - failures) << "/" << tests.size()
            << " tests passed\n";
  return failures ? 1 : 0;
}
