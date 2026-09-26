#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Clang detection
#ifdef __is_identifier
    // `_Float16` is always a keyword in Clang, but only usable on targets
    // that support it (e.g. not on WebAssembly/Emscripten)
    #ifdef __FLT16_MAX__
        #define ZA_FLOAT16_DETECTED
    #endif

    #if !__is_identifier(_Float32)
        #define ZA_FLOAT32_DETECTED
    #endif

    #if !__is_identifier(_Float64)
        #define ZA_FLOAT64_DETECTED
    #endif

    #if !__is_identifier(_Float128)
        #define ZA_FLOAT128_DETECTED
    #endif
#endif


////////////////////////////////////////////////////////////
// Other compilers detection
#ifdef __STDCPP_FLOAT16_T__
    #define ZA_FLOAT16_DETECTED
#endif

#ifdef __STDCPP_FLOAT32_T__
    #define ZA_FLOAT32_DETECTED
#endif

#ifdef __STDCPP_FLOAT64_T__
    #define ZA_FLOAT64_DETECTED
#endif

#ifdef __STDCPP_FLOAT128_T__
    #define ZA_FLOAT128_DETECTED
#endif


namespace za
{
////////////////////////////////////////////////////////////
// 16 bits floating point types
#ifdef ZA_FLOAT16_DETECTED
using F16 = decltype(0.f16);
#endif


////////////////////////////////////////////////////////////
// 32 bits floating point types
#ifdef ZA_FLOAT32_DETECTED
using F32 = decltype(0.f32);
#endif


////////////////////////////////////////////////////////////
// 64 bits floating point types
#ifdef ZA_FLOAT64_DETECTED
using F64 = decltype(0.f64);
#endif


////////////////////////////////////////////////////////////
// 128 bits floating point types
#ifdef ZA_FLOAT128_DETECTED
using F128 = decltype(0.f128);
#endif

} // namespace za


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Conditional extended floating-point aliases (`F16`, `F32`, `F64`, `F128`)
///
/// Detects compiler/standard library support for the C++23 extended
/// floating-point types and exposes them under short names. Each
/// `Fxx` alias is only defined when the matching `ZA_FLOATxx_DETECTED`
/// macro is available.
///
////////////////////////////////////////////////////////////
