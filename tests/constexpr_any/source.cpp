#include <cassert>
#include <constexpr_any.h>
#include <stdexcept>
#include <string>
#include <vector>

#define ASSERT(x)     \
    if (!(x)) {       \
        return false; \
    }

namespace test_helpers {

    struct SmallSizeObject {};
    struct LargeSizeObject {};

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

    template < class T >
    constexpr bool HasType(const mr::any& Any) {
        return mr::any_cast< T >(&Any) != nullptr;
    }

    template < class T, class U >
    constexpr bool CheckValue(const mr::any& Any, U u) {
        auto val = mr::any_cast< T >(&Any);
        if (val) {
            bool comp = val->GetValue() == u;
            ASSERT(comp);
            return true;
        }
        return false;
    }

} // namespace test_helpers

namespace test {

    template < class Type1, class Type2 >
    constexpr bool TestCopy() { // Copy assign
        {
            mr::any       t1 { Type1 { 1 } };
            const mr::any t2 { Type2 { 2 } };

            t1 = t2; // const copy

            ASSERT(test_helpers::HasType< Type2 >(t1))
            ASSERT(test_helpers::HasType< Type2 >(t2))
            ASSERT(test_helpers::CheckValue< Type2 >(t1, 2))
            ASSERT(test_helpers::CheckValue< Type2 >(t2, 2))
        }

        // Copy from and to empty any
        {
            mr::any       t1;
            const mr::any t2 { Type1 { 5 } };

            t1 = t2;

            ASSERT(test_helpers::HasType< Type1 >(t1));
            ASSERT(test_helpers::HasType< Type1 >(t2));
            ASSERT(test_helpers::CheckValue< Type1 >(t1, 5));
            ASSERT(test_helpers::CheckValue< Type1 >(t2, 5));
        }
        {
            mr::any       t1 { Type2 { 6 } };
            const mr::any t2;

            t1 = t2;

            ASSERT(!test_helpers::HasType< Type2 >(t1));
            ASSERT(!test_helpers::HasType< Type2 >(t2));
        }

        // Assign to myself is no-op
        {
            mr::any t {};
            t = static_cast< mr::any& >(t);
            ASSERT(!t.has_value());
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

            ASSERT(a1.has_value());
            ASSERT(!a2.has_value());
            ASSERT(test_helpers::CheckValue< Type2 >(a1, 3));
        }

        // Move assign from and to empty any

        {
            mr::any t1;
            mr::any t2 { Type1 { 1 } };

            ASSERT(!t1.has_value());
            t1 = std::move(t2);

            ASSERT(t1.has_value());
            ASSERT(!t2.has_value());
            ASSERT(test_helpers::CheckValue< Type1 >(t1, 1));
        }
        {
            mr::any t1 { Type2 { 2 } };
            mr::any t2;

            t1 = std::move(t2);

            ASSERT(!test_helpers::HasType< Type2 >(t1));
            ASSERT(!t1.has_value());
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
        a1.emplace< T >(1);
        a2.emplace< T >(1.f, 2);
        a3.emplace< T >({ 4, 3, 2 }, 1.f, 2);

        ASSERT(test_helpers::CheckValue< T >(a1, 1))
        ASSERT(test_helpers::CheckValue< T >(a2, 2))
        ASSERT(test_helpers::CheckValue< T >(a3, 4))

        return true;
    }

    template < class T >
    constexpr bool CastTests() {
        const mr::any a1 = T {};

        auto valuePtr = mr::any_cast< T >(&a1);
        ASSERT(*valuePtr == T {})
        ASSERT(test_helpers::HasType< T >(a1))
        ASSERT(a1.has_value())

        if constexpr (std::is_fundamental_v< T >) {
            mr::any a2     = static_cast< T >(5);
            auto    value2 = mr::any_cast< T >(a2);
            ASSERT(value2 == static_cast< T >(5))
            ASSERT(a2.has_value())

            mr::any a3     = static_cast< T >(10);
            auto    value3 = mr::any_cast< T&& >(std::move(a3));
            ASSERT(value3 == static_cast< T >(10))
        } else {
            mr::any a2     = T { 5 };
            auto    value2 = mr::any_cast< T >(a2);
            ASSERT(value2 == T { 5 })
            ASSERT(a2.has_value())

            mr::any a3     = T { 10 };
            auto    value3 = mr::any_cast< T >(std::move(a3));
            ASSERT(value3 == T { 10 })
        }

        return true;
    }

#ifndef _MSC_VER // Reported: https://developercommunity.visualstudio.com/t/constexpr-polymorphism-does-not-work-when-accessin/1634413?
    static_assert(TestCopy< test_helpers::Object< test_helpers::SmallSizeObject >,
                            test_helpers::Object< test_helpers::SmallSizeObject > >());
    static_assert(TestCopy< test_helpers::Object< test_helpers::LargeSizeObject >,
                            test_helpers::Object< test_helpers::LargeSizeObject > >());
    static_assert(TestCopy< test_helpers::Object< test_helpers::SmallSizeObject >,
                            test_helpers::Object< test_helpers::LargeSizeObject > >());
    static_assert(TestCopy< test_helpers::Object< test_helpers::LargeSizeObject >,
                            test_helpers::Object< test_helpers::SmallSizeObject > >());
#endif // _MSC_VER

    static_assert(TestMove< test_helpers::Object< test_helpers::SmallSizeObject >,
                            test_helpers::Object< test_helpers::SmallSizeObject > >());
    static_assert(TestMove< test_helpers::Object< test_helpers::LargeSizeObject >,
                            test_helpers::Object< test_helpers::LargeSizeObject > >());
    static_assert(TestMove< test_helpers::Object< test_helpers::SmallSizeObject >,
                            test_helpers::Object< test_helpers::LargeSizeObject > >());
    static_assert(TestMove< test_helpers::Object< test_helpers::LargeSizeObject >,
                            test_helpers::Object< test_helpers::SmallSizeObject > >());

    static_assert(MoveAssignNoExcept());

    static_assert(EmplaceTests< test_helpers::Object< test_helpers::SmallSizeObject > >());
    static_assert(EmplaceTests< test_helpers::Object< test_helpers::LargeSizeObject > >());

    static_assert(CastTests< int >());
    static_assert(CastTests< float >());
#ifdef __cpp_lib_constexpr_string
    static_assert(CastTests< std::string >());
#endif // __cpp_lib_constexpr_string
#ifdef __cpp_lib_constexpr_vector
    static_assert(CastTests< std::vector< int > >());
#endif // __cpp_lib_constexpr_vector

} // namespace test

int main() {
}

