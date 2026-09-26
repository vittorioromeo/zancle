#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Tst/Detail/StringifyValue.hpp"
#include "Tst/TstFwd.hpp"

#include "Zancle/Base/Pragma.hpp"

#include "Zancle/Trait/IsArray.hpp"


namespace tst::detail
{
////////////////////////////////////////////////////////////
// Dependent false for the forbidden operator overloads below; this keeps
// the diagnostic lazy until a problematic assertion expression is used.
template <typename>
inline constexpr bool deferredFalse = false;


////////////////////////////////////////////////////////////
struct Result
{
    bool passed;

    // Implicit converting ctor so the SFINAE-fallback path (where the
    // user's `lhs op rhs` returns plain `bool` after our templated
    // operator drops out) still feeds into `const Result =` cleanly.
    constexpr /*implicit*/ Result(bool p) noexcept : passed{p} // NOLINT(google-explicit-constructor)
    {
    }


// NOLINTBEGIN(bugprone-macro-parentheses)
// Match doctest's behavior: once decomposition produced a Result, any
// further operator chaining means the assertion expression needs rewriting.
#define ZA_TST_FORBID_RESULT_OP(op)                                                                     \
    template <typename R>                                                                               \
    Result& operator op(const R&)                                                                       \
    {                                                                                                   \
        static_assert(deferredFalse<R>, "Expression Too Complex Please Rewrite As Binary Comparison!"); \
        return *this;                                                                                   \
    }

    ZA_TST_FORBID_RESULT_OP(&)
    ZA_TST_FORBID_RESULT_OP(^)
    ZA_TST_FORBID_RESULT_OP(|)
    ZA_TST_FORBID_RESULT_OP(&&)
    ZA_TST_FORBID_RESULT_OP(||)
    ZA_TST_FORBID_RESULT_OP(==)
    ZA_TST_FORBID_RESULT_OP(!=)
    ZA_TST_FORBID_RESULT_OP(<)
    ZA_TST_FORBID_RESULT_OP(>)
    ZA_TST_FORBID_RESULT_OP(<=)
    ZA_TST_FORBID_RESULT_OP(>=)
    ZA_TST_FORBID_RESULT_OP(=)
    ZA_TST_FORBID_RESULT_OP(+=)
    ZA_TST_FORBID_RESULT_OP(-=)
    ZA_TST_FORBID_RESULT_OP(*=)
    ZA_TST_FORBID_RESULT_OP(/=)
    ZA_TST_FORBID_RESULT_OP(%=)
    ZA_TST_FORBID_RESULT_OP(<<=)
    ZA_TST_FORBID_RESULT_OP(>>=)
    ZA_TST_FORBID_RESULT_OP(&=)
    ZA_TST_FORBID_RESULT_OP(^=)
    ZA_TST_FORBID_RESULT_OP(|=)

#undef ZA_TST_FORBID_RESULT_OP
    // NOLINTEND(bugprone-macro-parentheses)
};


////////////////////////////////////////////////////////////
/// \brief Render the operands of a failed assertion into the runner's
/// scratch buffer (consumed by the failure banner). Bodies in
/// `Decompose.cpp`. Declared ahead of `ExpressionLhs` so the comparison
/// operators below resolve them by ordinary lookup.
////////////////////////////////////////////////////////////
void recordDecomposition(const char* opStr, const void* lhs, StringifyFn lhsFn, const void* rhs, StringifyFn rhsFn) noexcept;

void recordDecompositionUnary(const void* lhs, StringifyFn lhsFn) noexcept;


////////////////////////////////////////////////////////////
template <typename L>
struct ExpressionLhs
{
    const L&   lhs;
    AssertKind kind;

    ExpressionLhs(const L& in, AssertKind k) noexcept : lhs{in}, kind{k}
    {
    }


    // SFINAE-guarded so the operator drops out of overload resolution when
    // `lhs op rhs` is ill-formed or ambiguous in the calling context.
    // Matches doctest's `SFINAE_OP` trick: when the templated member
    // operator is removed, the compiler falls through to the implicit
    // conversion `operator L()` below.

// NOLINTBEGIN(bugprone-macro-parentheses)
// (clang-format would otherwise concatenate adjacent string literals
// inside `_Pragma(...)` arguments and split them across lines, which
// produces ill-formed code -- `_Pragma` requires a single string literal
// token, not a concatenation expression.)
#define ZA_TST_DECL_CMP_OP(op)                                                                           \
                                                                                                         \
    ZA_PRAGMA(GCC diagnostic push);                                                                      \
    /* Suppress the unknown-pragma warning before mentioning Clang-only options below. */                \
    ZA_PRAGMA(GCC diagnostic ignored "-Wpragmas");                                                       \
    ZA_PRAGMA(GCC diagnostic ignored "-Wunknown-warning-option");                                        \
    ZA_PRAGMA(GCC diagnostic ignored "-Wsign-compare");                                                  \
    ZA_PRAGMA(GCC diagnostic ignored "-Wsign-conversion");                                               \
    ZA_PRAGMA(GCC diagnostic ignored "-Wimplicit-int-float-conversion");                                 \
    ZA_PRAGMA(GCC diagnostic ignored "-Wdouble-promotion");                                              \
    ZA_PRAGMA(GCC diagnostic ignored "-Wfloat-equal");                                                   \
    ZA_PRAGMA(GCC diagnostic ignored "-Wconversion");                                                    \
                                                                                                         \
    template <typename R>                                                                                \
    auto operator op(const R& rhs)                                                                       \
        const->decltype((void)(static_cast<const L&>(lhs) op static_cast<const R&>(rhs)), Result{false}) \
    {                                                                                                    \
        bool res = static_cast<bool>(lhs op rhs);                                                        \
        if (kind == AssertKind::CheckFalse || kind == AssertKind::RequireFalse)                          \
            res = !res;                                                                                  \
        if (!res)                                                                                        \
            recordDecomposition(" " #op " ", &lhs, &stringifyThunk<L>, &rhs, &stringifyThunk<R>);        \
        return Result{res};                                                                              \
    }                                                                                                    \
                                                                                                         \
    ZA_PRAGMA(GCC diagnostic pop);

    ZA_TST_DECL_CMP_OP(==)
    ZA_TST_DECL_CMP_OP(!=)
    ZA_TST_DECL_CMP_OP(>)
    ZA_TST_DECL_CMP_OP(<)
    ZA_TST_DECL_CMP_OP(>=)
    ZA_TST_DECL_CMP_OP(<=)

#undef ZA_TST_DECL_CMP_OP
    // NOLINTEND(bugprone-macro-parentheses)


// NOLINTBEGIN(bugprone-macro-parentheses)
// Reject low-precedence/logical operators before the fallback conversion can
// turn CHECK_FALSE(a && b) into a plain bool expression with inverted logic lost.
#define ZA_TST_FORBID_EXPR_LHS_OP(op)                                                                   \
    template <typename R>                                                                               \
    ExpressionLhs& operator op(const R&)                                                                \
    {                                                                                                   \
        static_assert(deferredFalse<R>, "Expression Too Complex Please Rewrite As Binary Comparison!"); \
        return *this;                                                                                   \
    }

    ZA_TST_FORBID_EXPR_LHS_OP(&)
    ZA_TST_FORBID_EXPR_LHS_OP(^)
    ZA_TST_FORBID_EXPR_LHS_OP(|)
    ZA_TST_FORBID_EXPR_LHS_OP(&&)
    ZA_TST_FORBID_EXPR_LHS_OP(||)
    ZA_TST_FORBID_EXPR_LHS_OP(=)
    ZA_TST_FORBID_EXPR_LHS_OP(+=)
    ZA_TST_FORBID_EXPR_LHS_OP(-=)
    ZA_TST_FORBID_EXPR_LHS_OP(*=)
    ZA_TST_FORBID_EXPR_LHS_OP(/=)
    ZA_TST_FORBID_EXPR_LHS_OP(%=)
    ZA_TST_FORBID_EXPR_LHS_OP(<<=)
    ZA_TST_FORBID_EXPR_LHS_OP(>>=)
    ZA_TST_FORBID_EXPR_LHS_OP(&=)
    ZA_TST_FORBID_EXPR_LHS_OP(^=)
    ZA_TST_FORBID_EXPR_LHS_OP(|=)
    ZA_TST_FORBID_EXPR_LHS_OP(<<)
    ZA_TST_FORBID_EXPR_LHS_OP(>>)

#undef ZA_TST_FORBID_EXPR_LHS_OP
    // NOLINTEND(bugprone-macro-parentheses)


    // Conversion to Result for unary expressions like CHECK(b)
    operator Result() const // NOLINT(google-explicit-constructor)
    {
        bool res = static_cast<bool>(lhs);
        if (kind == AssertKind::CheckFalse || kind == AssertKind::RequireFalse)
            res = !res;
        if (!res)
            recordDecompositionUnary(&lhs, &stringifyThunk<L>);
        return Result{res};
    }


    // Fallback implicit conversion to L. Lets the compiler retry the
    // ambient `lhs op rhs` overload set when our templated operator
    // is excluded by SFINAE -- gives the user a useful diagnostic
    // instead of a "no operator" error. Returning an array type is
    // ill-formed, so SFINAE-guard the function for non-array L only.
    template <typename U = L>
    operator U() const noexcept // NOLINT(google-explicit-constructor)
        requires(!::za::isArray<U>)
    {
        return lhs;
    }
};


////////////////////////////////////////////////////////////
class ExpressionDecomposer
{
public:
    AssertKind kind;

    explicit ExpressionDecomposer(AssertKind k) noexcept : kind{k}
    {
    }

    template <typename L>
    ExpressionLhs<L> operator<<(const L& operand) const noexcept
    {
        return ExpressionLhs<L>(operand, kind);
    }
};


////////////////////////////////////////////////////////////
/// \brief Out-of-line glue. Records the assertion result with the runner,
/// reports a failure (with just the `#expr` string -- no value
/// decomposition; see header comment) if needed, and returns `true` for
/// "passed". Body lives once in `Decompose.cpp`.
////////////////////////////////////////////////////////////
bool handleAssertion(Result res, AssertKind kind, const char* file, int line, const char* exprStr);

} // namespace tst::detail


////////////////////////////////////////////////////////////
/// \file
///
/// Expression decomposition for CHECK / REQUIRE.
///
/// Same Catch/doctest trick: the `CHECK(a == b)` expression is wrapped
/// in `Decomposer{} << (a == b)` -- `operator<<` grabs the LHS via
/// `ExpressionLhs<L>`, then `ExpressionLhs::operator==` (and friends)
/// capture the RHS, evaluate the comparison, and return a tiny
/// `Result` carrying just the pass/fail bit.
///
/// On failure only (a cold path), the comparison operator hands the
/// operand addresses plus per-type `stringifyThunk<T>` function pointers
/// to `recordDecomposition`, which renders `<lhs> <op> <rhs>` into the
/// runner's scratch buffer for the failure banner. The thunks bottom out
/// in `tst::detail::stringifyValue` (see `Tst/Detail/StringifyValue.hpp`),
/// whose sink is a raw `char*` -- so this header pulls neither `String`
/// nor `Fmt`, and a test TU pays only a trivial per-operand-type thunk
/// instantiation.
////////////////////////////////////////////////////////////
