#include <constexpr_any.h>
#include <string>
#include <vector>

struct S {
    int data;

    constexpr S(int in) : data(in) {};
    constexpr ~S() = default;

    constexpr operator int() const noexcept {
        return data;
    }
};

constexpr mr::constexpr_any get_int_any() {
    return 4;
}

inline static constexpr const char* string = "13";
constexpr mr::constexpr_any         get_string_any() {
    return string;
}

constexpr mr::constexpr_any get_S_any() {
    return S { 1 };
}

constexpr int constexpr_main() {
    using namespace mr;

    constexpr_any any_as_int    = get_int_any();
    constexpr_any any_as_string = get_string_any();
    constexpr_any any_as_S      = get_S_any();

    auto string_value = any_as_string.cast< const char* >();
    bool true_1       = '1' == (*string_value)[0];
    bool true_3       = '3' == (*string_value)[1];
    bool false_2      = '2' == *string_value[0];

    auto int_value = any_as_int.cast< int >();

    auto S_value = any_as_S.cast< S >();

    constexpr_any any_as_inplace_S { std::in_place_type< S >, 3 };
    auto          inplace_S_value = any_as_inplace_S.cast< S >();

    // auto error = any_as_inplace_S.cast< int >();

    return *int_value + S_value->data + true_1 + true_3 + false_2 + inplace_S_value->data;
    // return 4 + 1 + 1 + 1 + 0 + 3 = 10
}

int main() {
    static_assert(10 == constexpr_main());
}
