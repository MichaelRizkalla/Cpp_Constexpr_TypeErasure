#include <algorithm>
#include <array>
#include <cassert>
#include <constexpr_any.h>
#include <stdexcept>
#include <string>
#include <vector>

#define CONSTEXPR_ASSERT(x) \
    if (!(x)) {             \
        return false;       \
    }

namespace test_helpers {

    struct SmallSizeObject {};
    struct LargeSizeObject {};

    struct UseTemplateCtor {};

    template < class Type >
    struct Object {
      public:
        constexpr int GetValue() const noexcept {
            return value[0];
        }

        constexpr explicit Object(int val = 0) {
            value[0] = val;
        }

        constexpr explicit Object(float, int val) {
            value[0] = val;
        }

        constexpr explicit Object(std::initializer_list< int > vals, float, int) {
            value[0] = *vals.begin();
        }

        template < class T >
        constexpr explicit Object(UseTemplateCtor, T val) {
            value[0] = static_cast< int >(val);
        }

        constexpr Object(Object const& rhs) noexcept {
            value[0] = rhs.value[0];
        }

        constexpr Object(Object& rhs) noexcept {
            value[0] = rhs.value[0];
        }

        constexpr Object(Object&& rhs) noexcept {
            value[0]     = rhs.value[0];
            rhs.value[0] = 0;
        }

        constexpr ~Object() {
            value[0] = -1;
        }

        constexpr Object& operator=(Object const&) = delete;
        constexpr Object& operator=(Object&&) = delete;

      private:
        static_assert(std::is_same_v< Type, SmallSizeObject > || std::is_same_v< Type, LargeSizeObject >);

        static constexpr int Values =
            std::conditional_t< std::is_same_v< Type, SmallSizeObject >, std::integral_constant< int, 1 >,
                                std::integral_constant< int, 20 > >::value;
        int value[Values] { 0 };
    };

    struct ConvertibleToInt {
        constexpr ConvertibleToInt(int val) : value(val) {
        }
        constexpr ~ConvertibleToInt() = default;

        constexpr operator int() {
            return value;
        }

        int value;
    };

    template < class T >
    constexpr bool HasType(const mr::any& Any) {
        return mr::any_cast< T >(&Any) != nullptr;
    }

    template < class T, class U >
    constexpr bool CheckValue(const mr::any& Any, U u) {
        auto val = mr::any_cast< T >(&Any);
        if (val) {
            bool comp = val->GetValue() == u;
            CONSTEXPR_ASSERT(comp);
            return true;
        }
        return false;
    }

} // namespace test_helpers

namespace test {

    template < class T >
    constexpr bool TestDestroy() {
        union TestType {
            int     b {};
            mr::any a;

            constexpr TestType() {};
            constexpr ~TestType() {};
        };

        TestType t {};
        std::construct_at(&t.a, T { 5 });
        CONSTEXPR_ASSERT(test_helpers::CheckValue< T >(t.a, 5))

#ifndef _MSC_VER
        std::destroy_at(&t.a);
#else  // _MSC_VER
        t.a = mr::any {};
#endif // _MSC_VER
        t.b = 10;
        return true;
    }

    template < class Type1, class Type2 >
    constexpr bool TestCopy() { // Copy assign
        {
            mr::any       t1 { Type1 { 1 } };
            const mr::any t2 { Type2 { 2 } };

            t1 = t2; // const copy

            if constexpr (!std::same_as< Type1, Type2 >) {
                CONSTEXPR_ASSERT(!test_helpers::HasType< Type1 >(t1))
            }
            CONSTEXPR_ASSERT(test_helpers::HasType< Type2 >(t1))
            CONSTEXPR_ASSERT(test_helpers::HasType< Type2 >(t2))
            CONSTEXPR_ASSERT(test_helpers::CheckValue< Type2 >(t1, 2))
            CONSTEXPR_ASSERT(test_helpers::CheckValue< Type2 >(t2, 2))
        }

        // Copy from and to empty any
        {
            mr::any       t1;
            const mr::any t2 { Type1 { 5 } };

            t1 = t2;

            CONSTEXPR_ASSERT(test_helpers::HasType< Type1 >(t1));
            CONSTEXPR_ASSERT(test_helpers::HasType< Type1 >(t2));
            CONSTEXPR_ASSERT(test_helpers::CheckValue< Type1 >(t1, 5));
            CONSTEXPR_ASSERT(test_helpers::CheckValue< Type1 >(t2, 5));
        }
        {
            mr::any       t1 { Type2 { 6 } };
            const mr::any t2;

            t1 = t2;

            CONSTEXPR_ASSERT(!test_helpers::HasType< Type2 >(t1));
            CONSTEXPR_ASSERT(!test_helpers::HasType< Type2 >(t2));
        }

        // Assign to myself is no-op
        {
            mr::any t {};
            t = static_cast< mr::any& >(t);
            CONSTEXPR_ASSERT(!t.has_value());
        }
        {
            mr::any t { Type1 { 1 } };

            t = static_cast< mr::any& >(t);

            t = static_cast< mr::any&& >(t);
        }
        {
            mr::any t { Type2 { 1 } };

            t = static_cast< mr::any& >(t);

            t = static_cast< mr::any&& >(t);
        }
        return true;
    }

    template < class Type1, class Type2 >
    constexpr bool TestMove() {
        // Move assign
        {
            const Type1 t1 { 1 };
            mr::any     a1 { t1 };
            Type2 const t2 { 3 };
            mr::any     a2 { t2 };

            a1 = std::move(a2);

            if constexpr (!std::same_as< Type1, Type2 >) {
                CONSTEXPR_ASSERT(!test_helpers::HasType< Type1 >(a1))
            }
            CONSTEXPR_ASSERT(a1.has_value());
            CONSTEXPR_ASSERT(!a2.has_value());
            CONSTEXPR_ASSERT(test_helpers::CheckValue< Type2 >(a1, 3));
        }

        // Move assign from and to empty any

        {
            mr::any t1;
            mr::any t2 { Type1 { 1 } };

            CONSTEXPR_ASSERT(!t1.has_value());
            t1 = std::move(t2);

            CONSTEXPR_ASSERT(t1.has_value());
            CONSTEXPR_ASSERT(!t2.has_value());
            CONSTEXPR_ASSERT(test_helpers::CheckValue< Type1 >(t1, 1));
        }
        {
            mr::any t1 { Type2 { 2 } };
            mr::any t2;

            t1 = std::move(t2);

            CONSTEXPR_ASSERT(!test_helpers::HasType< Type2 >(t1));
            CONSTEXPR_ASSERT(!t1.has_value());
        }

        return true;
    }

    constexpr bool MoveAssignNoExcept() {
        mr::any t1;
        mr::any t2;
        static_assert(noexcept(t1 = std::move(t2)), "any & operator=(any &&) must be noexcept");

        return true;
    }

    template < class T >
    constexpr bool EmplaceTests() {
        mr::any a1 {};
        mr::any a2 {};
        mr::any a3 {};
        mr::any a4 {};
        mr::any a5 {};
        a1.emplace< T >(1);
        a2.emplace< T >(1.f, 2);
        a3.emplace< T >({ 4, 3, 2 }, 1.f, 2);
        a4.emplace< T >(test_helpers::UseTemplateCtor {}, 5);
        a5.emplace< T >(test_helpers::UseTemplateCtor {}, test_helpers::ConvertibleToInt { 6 });

        CONSTEXPR_ASSERT(test_helpers::CheckValue< T >(a1, 1))
        CONSTEXPR_ASSERT(test_helpers::CheckValue< T >(a2, 2))
        CONSTEXPR_ASSERT(test_helpers::CheckValue< T >(a3, 4))
        CONSTEXPR_ASSERT(test_helpers::CheckValue< T >(a4, 5))
        CONSTEXPR_ASSERT(test_helpers::CheckValue< T >(a5, 6))

        return true;
    }

    template < class T >
    constexpr bool CastTests() {
        const mr::any a1 = T {};

        auto valuePtr = mr::any_cast< T >(&a1);
        CONSTEXPR_ASSERT(*valuePtr == T {})
        CONSTEXPR_ASSERT(test_helpers::HasType< T >(a1))
        CONSTEXPR_ASSERT(a1.has_value())

        if constexpr (std::is_fundamental_v< T >) {
            mr::any a2     = static_cast< T >(5);
            auto    value2 = mr::any_cast< T >(a2);
            CONSTEXPR_ASSERT(value2 == static_cast< T >(5))
            CONSTEXPR_ASSERT(a2.has_value())

            mr::any a3     = static_cast< T >(10);
            auto    value3 = mr::any_cast< T&& >(std::move(a3));
            CONSTEXPR_ASSERT(value3 == static_cast< T >(10))
        } else {
            mr::any a2     = T { 5 };
            auto    value2 = mr::any_cast< T >(a2);
            CONSTEXPR_ASSERT(value2 == T { 5 })
            CONSTEXPR_ASSERT(a2.has_value())

            mr::any a3     = T { 10 };
            auto    value3 = mr::any_cast< T >(std::move(a3));
            CONSTEXPR_ASSERT(value3 == T { 10 })
        }

        return true;
    }

    template < class T >
    constexpr T GetMaxValues(std::initializer_list< mr::any > values) {
        return mr::any_cast< T >(
            *std::max_element(values.begin(), values.end(), [](const mr::any& rhs, const mr::any& lhs) {
                const T rhs_t = mr::any_cast< T >(rhs);
                const T lhs_t = mr::any_cast< T >(lhs);
                return rhs_t < lhs_t;
            }));
    }

    template < class T >
    constexpr int Strlen(const mr::any& str /* any string type */) {
#ifdef __cpp_lib_constexpr_string
        if constexpr (std::is_same_v< T, std::string >) {
            return mr::any_cast< std::string >(&str)->size();
        } else
#endif // __cpp_lib_constexpr_string
            if constexpr (std::is_same_v< T, std::string_view >) {
            return mr::any_cast< std::string_view >(&str)->size();
        } else {
            auto myStr = mr::any_cast< const char* >(str);
            int  len   = 0;
            while (*myStr != '\0') {
                ++len;
                ++myStr;
            }
            return len;
        }
    }

} // namespace test

static_assert(test::TestDestroy< test_helpers::Object< test_helpers::SmallSizeObject > >());
static_assert(test::TestDestroy< test_helpers::Object< test_helpers::LargeSizeObject > >());

static_assert(test::TestCopy< test_helpers::Object< test_helpers::SmallSizeObject >,
                              test_helpers::Object< test_helpers::SmallSizeObject > >());
static_assert(test::TestCopy< test_helpers::Object< test_helpers::LargeSizeObject >,
                              test_helpers::Object< test_helpers::LargeSizeObject > >());
static_assert(test::TestCopy< test_helpers::Object< test_helpers::SmallSizeObject >,
                              test_helpers::Object< test_helpers::LargeSizeObject > >());
static_assert(test::TestCopy< test_helpers::Object< test_helpers::LargeSizeObject >,
                              test_helpers::Object< test_helpers::SmallSizeObject > >());

static_assert(test::TestMove< test_helpers::Object< test_helpers::SmallSizeObject >,
                              test_helpers::Object< test_helpers::SmallSizeObject > >());
static_assert(test::TestMove< test_helpers::Object< test_helpers::LargeSizeObject >,
                              test_helpers::Object< test_helpers::LargeSizeObject > >());
static_assert(test::TestMove< test_helpers::Object< test_helpers::SmallSizeObject >,
                              test_helpers::Object< test_helpers::LargeSizeObject > >());
static_assert(test::TestMove< test_helpers::Object< test_helpers::LargeSizeObject >,
                              test_helpers::Object< test_helpers::SmallSizeObject > >());

static_assert(test::MoveAssignNoExcept());

static_assert(test::EmplaceTests< test_helpers::Object< test_helpers::SmallSizeObject > >());
static_assert(test::EmplaceTests< test_helpers::Object< test_helpers::LargeSizeObject > >());

static_assert(test::CastTests< int >());
static_assert(test::CastTests< float >());
#ifdef __cpp_lib_constexpr_string
static_assert(test::CastTests< std::string >());
#endif // __cpp_lib_constexpr_string
#ifdef __cpp_lib_constexpr_vector
static_assert(test::CastTests< std::vector< int > >());
#endif // __cpp_lib_constexpr_vector
static_assert(test::CastTests< std::array< int, 10 > >());

static_assert(test::GetMaxValues< int >({ 1, 2, 3, 4, 9, 5, 6 }) == 9);
static_assert(test::GetMaxValues< std::uint64_t >({ std::uint64_t { 10 }, std::uint64_t { 2 }, std::uint64_t { 31 },
                                                    std::uint64_t { 44 }, std::uint64_t { 19 }, std::uint64_t { 75 },
                                                    std::uint64_t { 46 } }) == std::uint64_t { 75 });

static_assert(test::Strlen< const char* >("this is const char* string") == 26);
namespace {
    using namespace std::literals::string_view_literals;
    static_assert(test::Strlen< std::string_view >("this is std::string_view string"sv) == 31);
} // namespace

#ifdef __cpp_lib_constexpr_string
static_assert(test::Strlen< std::string >(std::string { "small string" }) == 12);
static_assert(test::Strlen< std::string >(std::string {
                  "A large string that will not fit into a small string optimization!" }) == 66);
#endif // __cpp_lib_constexpr_string

int main() {

    auto result = test::TestCopy< test_helpers::Object< test_helpers::SmallSizeObject >,
                                  test_helpers::Object< test_helpers::SmallSizeObject > >();
    assert(result);
    result = test::TestCopy< test_helpers::Object< test_helpers::LargeSizeObject >,
                             test_helpers::Object< test_helpers::LargeSizeObject > >();
    assert(result);
    result = test::TestCopy< test_helpers::Object< test_helpers::SmallSizeObject >,
                             test_helpers::Object< test_helpers::LargeSizeObject > >();
    assert(result);
    result = test::TestCopy< test_helpers::Object< test_helpers::LargeSizeObject >,
                             test_helpers::Object< test_helpers::SmallSizeObject > >();
    assert(result);

    result = test::TestMove< test_helpers::Object< test_helpers::SmallSizeObject >,
                             test_helpers::Object< test_helpers::SmallSizeObject > >();
    assert(result);
    result = test::TestMove< test_helpers::Object< test_helpers::LargeSizeObject >,
                             test_helpers::Object< test_helpers::LargeSizeObject > >();
    assert(result);
    result = test::TestMove< test_helpers::Object< test_helpers::SmallSizeObject >,
                             test_helpers::Object< test_helpers::LargeSizeObject > >();
    assert(result);
    result = test::TestMove< test_helpers::Object< test_helpers::LargeSizeObject >,
                             test_helpers::Object< test_helpers::SmallSizeObject > >();
    assert(result);

    assert(test::MoveAssignNoExcept());

    assert(test::EmplaceTests< test_helpers::Object< test_helpers::SmallSizeObject > >());
    assert(test::EmplaceTests< test_helpers::Object< test_helpers::LargeSizeObject > >());

    assert(test::CastTests< int >());
    assert(test::CastTests< float >());
#ifdef __cpp_lib_constexpr_string
    assert(test::CastTests< std::string >());
#endif // __cpp_lib_constexpr_string
#ifdef __cpp_lib_constexpr_vector
    assert(test::CastTests< std::vector< int > >());
#endif // __cpp_lib_constexpr_vector

    assert(test::GetMaxValues< int >({ 1, 2, 3, 4, 9, 5, 6 }) == 9);
    assert(test::GetMaxValues< std::uint64_t >({ std::uint64_t { 10 }, std::uint64_t { 2 }, std::uint64_t { 31 },
                                                 std::uint64_t { 44 }, std::uint64_t { 19 }, std::uint64_t { 75 },
                                                 std::uint64_t { 46 } }) == std::uint64_t { 75 });

    assert(test::Strlen< const char* >("this is const char* string") == 26);
    using namespace std::literals::string_view_literals;
    assert(test::Strlen< std::string_view >("this is std::string_view string"sv) == 31);

#ifdef __cpp_lib_constexpr_string
    assert(test::Strlen< std::string >(std::string { "small string" }) == 12);
    assert(test::Strlen< std::string >(
               std::string { "A large string that will not fit into a small string optimization!" }) == 66);
#endif // __cpp_lib_constexpr_string
}
