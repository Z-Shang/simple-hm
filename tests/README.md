# Algorithm W regression tests

This is an assertion-based, dependency-free companion to `main.cpp`, which
prints inference results but does not assert correctness. Run from the repository
root with a toolchain that supports the C++23 features used by `hm.hpp`
(the project README reports Clang 21):

```sh
clang++ -std=c++23 -Wall -Wextra tests/hm_regression.cpp -o hm_regression
./hm_regression
```

The executable runs all cases, prints PASS/FAIL for each, and exits nonzero if
any fail. It covers substitution composition, unification and occurs checks,
scheme generalization/instantiation, let-polymorphism, and inference failures.

**Known red tests on the current implementation:**

- `function unification composition`: the `type::Fn` branch combines substitutions
  in the wrong order, so `(a -> a) ~ (b -> int)` leaves `a` mapped to `b`
  rather than to `int` in a single substitution application.
- `let substitution propagation`: the `expr::Let` branch also reverses its
  substitution composition order. This leaves constraints inferred inside
  `let` unapplied to the enclosing lambda's parameter type.

These are deliberately test-only additions; this patch does not change
`hm.hpp`. The two red cases should turn green when the underlying issues are
fixed. There is no CI workflow in this patch, so run the executable locally.
