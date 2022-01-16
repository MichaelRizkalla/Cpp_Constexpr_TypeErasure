# Cpp_Constexpr_TypeErasure
 This repo records my trials to achieve type erasure in constexpr context.

### constexpr_any.h
 This header gives access to `mr::any` class which can be used in `constexpr` context using clang-11, MSVC v19.29, and GCC 11.2.

 `mr::any` internally uses a union as follows:
 ```C++
 union Data {
    std::max_align_t    align;
    ConstexprData       constexprData;
    char                any[sizeof(std::any)];
 };
 
 Data data {};
 ```
 That implementation aims to use the available `std::any` implementation at run-time, while using my internal `constexpr` implementation at compile-time if needed.
 The switch happens using `std::is_constant_evaluated()` whenever needed.
 The goal is to achieve similar code gen and performance at run-time when using `mr::any` compared to `std::any`.
