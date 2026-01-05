#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <expected>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hm {

using id_t = std::string;
using tv_t = std::string;

struct type;
struct expr;

struct prim {
  enum type { Bool, Int, Float, String };

  prim::type type_;

  union v {
    bool bool_;
    int int_;
    double float_;
    std::string str_;

    ~v() {}
  } v;

  std::string to_string(this auto const &self) noexcept {
    switch (self.type_) {
    case Bool: {
      return self.v.bool_ ? "true" : "false";
    }
    case Int: {
      return std::format("{}", self.v.int_);
    }
    case Float: {
      return std::format("{}", self.v.float_);
    }
    case String: {
      return std::format("\"{}\"", self.v.str_);
    }
    }
  }

  static auto mkFalse() {
    return std::shared_ptr<prim>(
        new prim{.type_ = Bool, .v = {.bool_ = false}});
  }
  static auto mkTrue() {
    return std::shared_ptr<prim>(new prim{.type_ = Bool, .v = {.bool_ = true}});
  }
  static auto mkInt(int i) {
    return std::shared_ptr<prim>(new prim{.type_ = Int, .v = {.int_ = i}});
  }
  static auto mkFloat(double n) {
    return std::shared_ptr<prim>(new prim{.type_ = Float, .v = {.float_ = n}});
  }
  static auto mkString(std::string s) {
    return std::shared_ptr<prim>(new prim{.type_ = String, .v = {.str_ = s}});
  }
};

using struct_t = std::vector<std::tuple<id_t, std::shared_ptr<type>>>;

struct struct_v {
  id_t type_;
  std::vector<std::tuple<id_t, std::shared_ptr<expr>>> fields_;
};

template <typename BVt, typename Et>
using abs_t = std::tuple<BVt, std::shared_ptr<Et>>;

template <typename Et>
using app_t = std::tuple<std::shared_ptr<Et>, std::shared_ptr<Et>>;

template <typename BVt, typename Et>
using let_t = std::tuple<BVt, std::shared_ptr<Et>, std::shared_ptr<Et>>;

template <typename Et>
using cond_t =
    std::tuple<std::shared_ptr<Et>, std::shared_ptr<Et>, std::shared_ptr<Et>>;

struct expr {
  enum kind { Var, Prim, Struct, Abs, App, Let, Tuple, Cond };

  kind kind_;

  union v {
    id_t var_;
    std::shared_ptr<prim> prim_;
    struct_v struct_;
    abs_t<id_t, expr> abs_;
    app_t<expr> app_;
    let_t<id_t, expr> let_;
    std::vector<std::shared_ptr<expr>> tuple_;
    cond_t<expr> cond_;

    ~v() {}
  } v;

  static auto mkVar(id_t v) {
    return std::shared_ptr<expr>(new expr{.kind_ = Var, .v = {.var_ = v}});
  }
  static auto mkPrim(std::shared_ptr<prim> p) {
    return std::shared_ptr<expr>(new expr{.kind_ = Prim, .v = {.prim_ = p}});
  }
  static auto mkStruct(struct_v v) {
    return std::shared_ptr<expr>(
        new expr{.kind_ = Struct, .v = {.struct_ = v}});
  }
  static auto mkAbs(id_t bv, std::shared_ptr<expr> body) {
    return std::shared_ptr<expr>(
        new expr{.kind_ = Abs, .v = {.abs_ = abs_t{bv, body}}});
  }
  static auto mkApp(std::shared_ptr<expr> e1, std::shared_ptr<expr> e2) {
    return std::shared_ptr<expr>(
        new expr{.kind_ = App, .v = {.app_ = app_t{e1, e2}}});
  }
  static auto mkLet(id_t bv, std::shared_ptr<expr> e1,
                    std::shared_ptr<expr> e2) {
    return std::shared_ptr<expr>(
        new expr{.kind_ = Let, .v = {.let_ = let_t{bv, e1, e2}}});
  }
  static auto mkTuple(std::vector<std::shared_ptr<expr>> t) {
    return std::shared_ptr<expr>(new expr{.kind_ = Tuple, .v = {.tuple_ = t}});
  }
  static auto mkCond(std::shared_ptr<expr> c, std::shared_ptr<expr> e1,
                     std::shared_ptr<expr> e2) {
    return std::shared_ptr<expr>(
        new expr{.kind_ = Cond, .v = {.cond_ = cond_t{c, e1, e2}}});
  }

  std::string to_string(this auto const &self) noexcept {
    switch (self.kind_) {
    case Var: {
      return self.v.var_;
    }
    case Prim: {
      return self.v.prim_->to_string();
    }
    case Struct: {
      std::string res = "#" + self.v.struct_.type_ + "{ ";
      for (const auto &[f, e] : self.v.struct_.fields_) {
        res += f + ": " + e->to_string() + " ";
      }
      res += "}";
      return res;
    }
    case Abs: {
      auto [in, out] = self.v.abs_;
      return std::format("(\\{} -> {})", in, out->to_string());
    }
    case App: {
      auto [e1, e2] = self.v.app_;
      return std::format("({} {})", e1->to_string(), e2->to_string());
    }
    case Let: {
      auto [bv, e1, e2] = self.v.let_;
      return std::format("let {} = {} in {}", bv, e1->to_string(),
                         e2->to_string());
    }
    case Tuple: {
      std::string res = "< ";
      for (const auto &e : self.v.tuple_) {
        res += std::format("[{}] ", e->to_string());
      }
      res += ">";
      return res;
    }
    case Cond: {
      auto [c, e1, e2] = self.v.cond_;
      return std::format("cond {} then {} else {}", c->to_string(),
                         e1->to_string(), e2->to_string());
    }
    }
  }
};

template <typename Tt>
using arrow_t = std::tuple<std::shared_ptr<Tt>, std::shared_ptr<Tt>>;

using subst_t = std::map<tv_t, std::shared_ptr<type>>;

struct type {
  enum kind { Prim, Struct, Tuple, Var, Fn };

  kind kind_;

  union t {
    prim::type prim_;
    struct_t struct_;
    std::vector<std::shared_ptr<type>> tuple_;
    tv_t var_;
    arrow_t<type> fn_;
    ~t() {}
  } t;

  std::string to_string(this auto const &self) noexcept {
    switch (self.kind_) {
    case Prim: {
      switch (self.t.prim_) {
      case prim::Int: {
        return "int";
      }
      case prim::Bool: {
        return "bool";
      }
      case prim::Float: {
        return "float";
      }
      case prim::String: {
        return "string";
      }
      }
    }
    case Struct: {
      std::string res = "{ ";
      for (const auto &[f, t] : self.t.struct_) {
        res += std::format("[{}: {}] ", f, t->to_string());
      }
      res += "}";
      return res;
    }
    case Tuple: {
      std::string res = "< ";
      for (const auto &e : self.t.tuple_) {
        res += std::format("[{}] ", e->to_string());
      }
      res += ">";
      return res;
    }
    case Var: {
      return self.t.var_;
    }
    case Fn: {
      auto [in, out] = self.t.fn_;
      return std::format("{} -> {}", in->to_string(), out->to_string());
    }
    }
  }

  static auto mkPrim(prim::type t) {
    return std::shared_ptr<type>(new type{.kind_ = Prim, .t = {.prim_ = t}});
  }

  static auto mkVar(tv_t n) {
    return std::shared_ptr<type>(new type{.kind_ = Var, .t = {.var_ = n}});
  }

  static auto mkFn(std::shared_ptr<type> in, std::shared_ptr<type> out) {
    return std::shared_ptr<type>(
        new type{.kind_ = Fn, .t = {.fn_ = {in, out}}});
  }

  static auto mkTuple(std::vector<std::shared_ptr<type>> t) {
    return std::shared_ptr<type>(new type{.kind_ = Tuple, .t = {.tuple_ = t}});
  }

  static auto mkStruct(struct_t s) {
    return std::shared_ptr<type>(
        new type{.kind_ = Struct, .t = {.struct_ = s}});
  }
};

using scheme_t = std::tuple<std::vector<tv_t>, std::shared_ptr<type>>;

static inline auto ftv(const std::shared_ptr<type> &ty) -> std::set<tv_t> {
  switch (ty->kind_) {
  case type::Prim:
    return {};
  case type::Var:
    return {ty->t.var_};
  case type::Struct: {
    std::set<tv_t> res;

    for (auto inner :
         ty->t.struct_ | std::views::values | std::views::transform(ftv)) {
      res.insert(inner.begin(), inner.end());
    }

    return res;
  }
  case type::Tuple: {
    std::set<tv_t> res;

    for (auto inner : ty->t.tuple_ | std::views::transform(ftv)) {
      res.insert(inner.begin(), inner.end());
    }

    return res;
  }
  case type::Fn: {
    auto [in, out] = ty->t.fn_;
    auto res = ftv(in);
    for (auto ot : ftv(out)) {
      res.insert(ot);
    }
    return res;
  }
  }
}

static inline auto ftv(const scheme_t &s) -> std::set<tv_t> {
  auto [vars, t] = s;
  auto tft = ftv(t);
  std::set<tv_t> res;
  std::ranges::set_difference(tft, vars, std::inserter(res, res.end()));

  return res;
}

static inline auto apply(const subst_t &subst,
                         const std::shared_ptr<type> &ty) {
  switch (ty->kind_) {
  case type::Var: {
    if (auto it = subst.find(ty->t.var_); it != subst.end()) {
      return it->second;
    }
    return ty;
  }
  case type::Fn: {
    auto [in, out] = ty->t.fn_;
    auto sin = apply(subst, in);
    auto sout = apply(subst, out);
    return type::mkFn(sin, sout);
  }
  case type::Tuple: {
    return type::mkTuple(ty->t.tuple_ | std::views::transform([&subst](auto t) {
                           return apply(subst, t);
                         }) |
                         std::ranges::to<std::vector>());
  }
  case type::Struct: {
    return type::mkStruct(ty->t.struct_ |
                          std::views::transform([&subst](auto f) {
                            return std::tuple<tv_t, std::shared_ptr<type>>{
                                std::get<0>(f), apply(subst, std::get<1>(f))};
                          }) |
                          std::ranges::to<std::vector>());
  }
  default: {
    return ty;
  }
  }
}

static inline auto apply(const subst_t &subst, const scheme_t &s) {
  subst_t filtered_s{subst};
  auto [vars, t] = s;
  for (const auto &v : vars) {
    filtered_s.erase(v);
  }
  return scheme_t{vars, apply(filtered_s, t)};
}

template <typename T>
concept types = requires(const T &t, const subst_t &s) {
  { ftv(t) } -> std::same_as<std::set<tv_t>>;
  { apply(s, t) } -> std::same_as<T>;
};

template <types T> static inline auto ftv(std::vector<T> ts) -> std::set<tv_t> {
  std::set<tv_t> res;
  for (const auto &t :
       ts | std::views::transform([](auto t) { return ftv(t); })) {
    res.insert(t.begin(), t.end());
  }
  return res;
}

template <types T>
static inline auto apply(const subst_t &subst, std::vector<T> ts)
    -> std::vector<T> {
  return ts |
         std::views::transform([&subst](auto t) { return apply(subst, t); }) |
         std::ranges::to<std::vector>();
}

static inline auto compose_subst(const subst_t &s1, const subst_t &s2) {
  subst_t res{s1};
  for (const auto &[k, v] : s2) {
    res.insert_or_assign(k, apply(s1, v));
  }
  return res;
}

using type_env_t = std::map<tv_t, scheme_t>;

static inline auto remove(const type_env_t &env, const tv_t &v) {
  type_env_t res{env};
  res.erase(v);
  return res;
}

static inline auto ftv(const type_env_t &env) {
  return ftv(env | std::views::values | std::ranges::to<std::vector>());
}

static inline auto apply(const subst_t &subst, const type_env_t &env) {
  // with ranges:
  return env | std::views::transform([&subst](const auto &t) {
           return decltype(t){std::get<0>(t), apply(subst, std::get<1>(t))};
         }) |
         std::ranges::to<type_env_t>();
  // without ranges:
  // type_env_t res;
  // for (const auto &[k, v] : env) {
  //   res.emplace(k, apply(subst, v));
  // }
  // return res;
}

static inline auto generalize(const type_env_t &env,
                              const std::shared_ptr<type> &ty) -> scheme_t {
  std::vector<tv_t> vars;
  std::ranges::set_difference(ftv(ty), ftv(env), std::back_inserter(vars));
  return {vars, ty};
}

struct ti_state {
  std::size_t supply_;
  subst_t subst_;

  ti_state() : supply_{0}, subst_{} {}
  ti_state(const ti_state &o) : supply_(o.supply_), subst_{o.subst_} {}

  auto new_tv(this auto &&self) {
    return type::mkVar(std::format("#:{}", self.supply_++));
  }

  auto new_tv(this auto &&self, std::string_view pref) {
    return type::mkVar(std::format("#:{}{}", pref, self.supply_++));
  }

  auto instantiate(this auto &&self, const scheme_t &s) {
    auto [vars, t] = s;
    auto subst =
        vars | std::views::transform([&self](auto k) {
          return std::pair<tv_t, std::shared_ptr<type>>{k, self.new_tv(k)};
        }) |
        std::ranges::to<subst_t>();
    return apply(subst, t);
  }
};

static inline auto occurs(const tv_t &v, const std::shared_ptr<type> &ty) {
  switch (ty->kind_) {
  case type::Prim: {
    return false;
  }
  case type::Var: {
    return v == ty->t.var_;
  }
  case type::Fn: {
    const auto [in, out] = ty->t.fn_;
    return occurs(v, in) || occurs(v, out);
  }
  case type::Tuple: {
    return std::ranges::count_if(ty->t.tuple_,
                                 [&v](auto t) { return occurs(v, t); }) > 0;
  }
  case type::Struct: {
    return std::ranges::count_if(ty->t.struct_ | std::views::values,
                                 [&v](auto t) { return occurs(v, t); }) > 0;
  }
  }
}

static inline auto var_bind(const tv_t &u, const std::shared_ptr<type> &ty)
    -> std::expected<subst_t, std::string> {
  if (ty->kind_ == type::Var && ty->t.var_ == u) {
    return subst_t{};
  }
  if (occurs(u, ty)) {
    return std::unexpected(
        std::format("Occur check failed for type var: {}", u));
  }
  return subst_t{{u, ty}};
}

static inline auto unify(const std::shared_ptr<type> &t1,
                         const std::shared_ptr<type> &t2)
    -> std::expected<subst_t, std::string> {
  auto t1k = t1->kind_;
  auto t2k = t2->kind_;
  if (t1k == type::Prim && t2k == type::Prim && t1->t.prim_ == t2->t.prim_) {
    return subst_t{};
  }
  if (t1k == type::Var) {
    return var_bind(t1->t.var_, t2);
  }
  if (t2k == type::Var) {
    return var_bind(t2->t.var_, t1);
  }
  if (t1k == type::Fn && t2k == type::Fn) {
    auto [in, out] = t1->t.fn_;
    auto [in_, out_] = t2->t.fn_;

    return unify(in, in_).and_then([&](auto s1) {
      return unify(apply(s1, out), apply(s1, out_))
          .and_then([&](auto s2) -> std::expected<subst_t, std::string> {
            return compose_subst(s1, s2);
          });
    });
  }
  if (t1k == type::Tuple && t2k == type::Tuple) {
    auto t1t = t1->t.tuple_;
    auto t2t = t2->t.tuple_;
    if (t1t.size() != t2t.size()) {
      return std::unexpected("Tuple sizes do not match");
    }
    subst_t subst;

    for (const auto &[t1e, t2e] : std::views::zip(t1t, t2t)) {
      auto s = unify(apply(subst, t1e), apply(subst, t2e));
      if (!s.has_value()) {
        return s;
      }
      subst = compose_subst(s.value(), subst);
    }

    return subst;
  }

  if (t1k == type::Struct && t2k == type::Struct) {
    auto t1t = t1->t.struct_;
    auto t2t = t2->t.struct_;
    if (t1t.size() != t2t.size()) {
      return std::unexpected("Tuple sizes do not match");
    }
    subst_t subst;

    for (const auto &[t1e, t2e] : std::views::zip(t1t, t2t)) {
      if (auto f1 = std::get<0>(t1e), f2 = std::get<0>(t2e); f1 != f2) {
        return std::unexpected(
            std::format("Field names do not match: {} {}", f1, f2));
      }
      auto s =
          unify(apply(subst, std::get<1>(t1e)), apply(subst, std::get<1>(t2e)));
      if (!s.has_value()) {
        return s;
      }
      subst = compose_subst(s.value(), subst);
    }

    return subst;
  }

  return std::unexpected(std::format("Failed to unify {} and {}",
                                     t1->to_string(), t2->to_string()));
}

static inline auto ti(std::shared_ptr<ti_state> &state, const type_env_t &env,
                      const std::shared_ptr<expr> &exp)
    -> std::expected<std::tuple<subst_t, std::shared_ptr<type>>, std::string> {
  // std::cout << "Curr Env: " << std::endl;
  // for (const auto &[k, v] : env) {
  //   auto [tv, t] = v;
  //   std::cout << "k: " << k << " v: [ ";
  //   for (const auto &tv_ : tv) {
  //     std::cout << tv_ << " ";
  //   }
  //   std::cout << "] " << t->to_string() << std::endl;
  // }
  // std::cout << "Expr: " << exp->to_string() << std::endl;

  switch (exp->kind_) {
  case expr::Prim:
    return std::tuple{subst_t{}, type::mkPrim(exp->v.prim_->type_)};
  case expr::Var: {
    if (auto it = env.find(exp->v.var_); it != env.end()) {
      auto t = state->instantiate(it->second);
      return std::tuple{subst_t{}, t};
    }
    return std::unexpected(std::format("Var: {} is not bound", exp->v.var_));
  }
  case expr::Abs: {
    auto [para, body] = exp->v.abs_;
    auto tv = state->new_tv(para);
    auto filtered_env{env};
    filtered_env.insert_or_assign(para, scheme_t{std::vector<tv_t>{}, tv});
    return ti(state, filtered_env, body)
        .and_then([&](auto r) -> std::expected<decltype(r), std::string> {
          auto [subst, t] = r;
          return std::tuple{subst, type::mkFn(apply(subst, tv), t)};
        });
  }
  case expr::App: {
    auto [e1, e2] = exp->v.app_;
    auto tv = state->new_tv("a");
    return ti(state, env, e1)
        .and_then([&](auto r) -> std::expected<decltype(r), std::string> {
          auto [s1, t1] = r;
          // std::cout << "E1 type: " << t1->to_string() << std::endl;
          return ti(state, apply(s1, env), e2)
              .and_then(
                  [&](auto r_) -> std::expected<decltype(r_), std::string> {
                    auto [s2, t2] = r_;
                    // std::cout << "E2 type: " << t2->to_string() << std::endl;
                    return unify(apply(s2, t1), type::mkFn(t2, tv))
                        .and_then(
                            [&](auto s3)
                                -> std::expected<decltype(r_), std::string> {
                              // std::cout << "Unified S: " << std::endl;
                              // for (const auto &[k, v] : s3) {
                              //   std::cout << k << " => " << v->to_string() <<
                              //   std::endl;
                              // }
                              return std::tuple{
                                  compose_subst(s3, compose_subst(s2, s1)),
                                  apply(s3, tv)};
                            });
                  });
        });
  }
  case expr::Let: {
    auto [x, e1, e2] = exp->v.let_;
    return ti(state, env, e1)
        .and_then([&](auto r) -> std::expected<decltype(r), std::string> {
          auto [s1, t1] = r;
          auto t_ = generalize(apply(s1, env), t1);
          auto filtered_env{env};
          filtered_env.insert_or_assign(x, t_);
          return ti(state, apply(s1, filtered_env), e2)
              .and_then(
                  [&](auto r_) -> std::expected<decltype(r_), std::string> {
                    auto [s2, t2] = r_;
                    return std::tuple{compose_subst(s1, s2), t2};
                  });
        });
  }
  case expr::Tuple: {
    subst_t subst;
    type_env_t cenv{env};

    std::vector<std::shared_ptr<type>> types;

    for (const auto &e : exp->v.tuple_) {
      auto inf = ti(state, cenv, e);
      if (!inf.has_value()) {
        return inf;
      }
      auto [s, t] = inf.value();
      subst = compose_subst(s, subst);
      cenv = apply(s, cenv);
      types.push_back(apply(subst, t));
    }

    return std::tuple{subst, type::mkTuple(types)};
  }
  case expr::Struct: {
    subst_t subst;
    type_env_t cenv{env};

    auto struct_ctor = env.find(exp->v.struct_.type_);
    if (struct_ctor == env.end()) {
      return std::unexpected(std::format("Struct constructor undefined for: {}",
                                         exp->v.struct_.type_));
    }

    auto struct_scheme = struct_ctor->second;

    if (exp->v.struct_.fields_.size() !=
        std::get<1>(struct_scheme)->t.struct_.size()) {
      return std::unexpected(
          std::format("Struct size mismatch for {}, expected: {}, got: {}",
                      exp->v.struct_.type_, exp->v.struct_.fields_.size(),
                      std::get<1>(struct_scheme)->t.struct_.size()));
    }

    auto struct_ty = state->instantiate(struct_scheme);
    std::vector<std::tuple<id_t, std::shared_ptr<type>>> types;

    for (const auto &[fd, e] :
         std::views::zip(struct_ty->t.struct_, exp->v.struct_.fields_)) {
      if (std::get<0>(fd) != std::get<0>(e)) {
        return std::unexpected(std::format("Field name: {} != {}",
                                           std::get<0>(fd), std::get<0>(e)));
      }
      auto inf = ti(state, cenv, std::get<1>(e));
      if (!inf.has_value()) {
        return inf;
      }
      auto [s, t] = inf.value();
      auto mgu = unify(std::get<1>(fd), t);
      if (!mgu.has_value()) {
        return std::unexpected(mgu.error());
      }
      subst = compose_subst(mgu.value(), compose_subst(s, subst));
      cenv = apply(subst, cenv);
      types.push_back({std::get<0>(fd), apply(subst, t)});
    }
    return std::tuple{subst, apply(subst, struct_ty)};
  }
  case expr::Cond: {
    auto [c, e1, e2] = exp->v.cond_;

    return ti(state, env, c)
        .and_then([&](auto r) -> std::expected<decltype(r), std::string> {
          auto [cs, ct] = r;
          return unify(ct, type::mkPrim(prim::Bool)).and_then([&](auto cs_) {
            return ti(state, apply(compose_subst(cs_, cs), env), e1)
                .and_then([&](auto r1) {
                  auto [s1, t1] = r1;
                  return ti(state,
                            apply(compose_subst(s1, compose_subst(cs_, cs)),
                                  env),
                            e2)
                      .and_then([&](auto r2) {
                        auto [s2, t2] = r2;
                        auto s = compose_subst(
                            s2, compose_subst(s1, compose_subst(cs_, cs)));
                        return unify(apply(s, t1), apply(s, t2))
                            .and_then(
                                [&](auto us)
                                    -> std::expected<decltype(r), std::string> {
                                  auto res_s = compose_subst(us, s);
                                  return std::tuple{res_s, apply(res_s, t1)};
                                });
                      });
                });
          });
        });
  }
  }
}

static inline auto run_ti(const std::shared_ptr<expr> &exp,
                          const type_env_t &env) {
  auto state = std::make_shared<ti_state>();
  return ti(state, env, exp);
}

static inline std::string show_scheme(const scheme_t &s) {
  auto [bv, t] = s;
  std::string res = "[ ";
  for (const auto &v : bv) {
    res += v + " ";
  }
  res += "] => " + t->to_string();

  return res;
}

} // namespace hm
