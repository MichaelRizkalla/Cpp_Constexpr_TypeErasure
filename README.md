# Cpp_Constexpr_TypeErasure
 This repo records my trials to achieve type erasure in constexpr context

### constexpr_function
 This class can be compiled in `constexpr` context only using clang, tested on clang 11. The code does not compile using MSVC19 or gcc-11. My current goal is to understand which behaviour among the three compiler is what the standard C++ defines.
