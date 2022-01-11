# Cpp_Constexpr_TypeErasure
 This repo records my trials to achieve type erasure in constexpr context.

### constexpr_any.h
 This header gives access to `mr::any` class which can be used in `constexpr` context using clang-11 (given it's destroyed before exiting `constexpr`) but does not compile on latest MSVC or g++-11.
 
 A bug report for MSVC was filed [here](https://developercommunity.visualstudio.com/t/constexpr-polymorphism-does-not-work-when-accessin/1634413?) to be investigate.

 `mr::any` internally uses a union as follows:
 ```C++
 union Data {
    std::max_align_t       align;
    detail::any_type_base* ptr;
    char                   any[sizeof(std::any)];
 };
 
 Data data {};
 ```
 That implementation aims to use the available `std::any` implementation at run-time, while using my internal `constexpr` implementation at compile-time if needed.
 The switch happens using `std::is_constant_evaluated()` whenever needed.
