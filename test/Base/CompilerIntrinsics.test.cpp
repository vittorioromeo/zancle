#include "Tst/Tst.hpp"

#include "Zancle/Base/CpuRelax.hpp"
#include "Zancle/Base/IsConstantEvaluated.hpp"


namespace
{
namespace CompilerIntrinsicsTest // for unity builds
{
////////////////////////////////////////////////////////////
[[nodiscard]] constexpr int evaluationKind() noexcept
{
    return ZA_IS_CONSTANT_EVALUATED() ? 1 : 2;
}

} // namespace CompilerIntrinsicsTest
} // namespace


TEST_CASE("[Base] Base/IsConstantEvaluated.hpp")
{
    using CompilerIntrinsicsTest::evaluationKind;

    STATIC_CHECK(evaluationKind() == 1);

    int kind = evaluationKind(); // not `const`: the initializer is not manifestly constant-evaluated
    CHECK(kind == 2);
}


TEST_CASE("[Base] Base/CpuRelax.hpp")
{
    // Only a hint: must compile to something harmless in any statement context
    for (int i = 0; i < 16; ++i)
        ZA_CPU_RELAX();

    if (true)
        ZA_CPU_RELAX();
    else
        ZA_CPU_RELAX();
}
