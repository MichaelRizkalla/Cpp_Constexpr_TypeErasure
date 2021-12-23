# Cpp_Constexpr_TypeErasure
 This repo records my trials to achieve type erasure in constexpr context.
 Please note that current implementations are usable only in `constexpr` context and if used in runtime will impose a performance penalty.

### constexpr_function
 This class can be compiled in `constexpr` context only using clang, tested on clang 13. The code does not compile using MSVC19 or gcc-11. My current goal is to understand which behaviour among the three compilers is what the standard C++ defines.

### constexpr_any
 This class can be compiled in `constexpr` context using clang-13 and MSVC compilers. It does not compile on gcc-11.
