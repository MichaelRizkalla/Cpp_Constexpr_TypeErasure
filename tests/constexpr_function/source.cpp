#include <cassert>
#include <constexpr_any.h>
#include <constexpr_function.h>
#include <functional>
#include <iostream>
#include <vector>

#define CONSTEXPR_ASSERT(x) \
    if (!(x)) {             \
        return false;       \
    }

namespace test {

    template < class T >
    constexpr int Do(int lhs, int rhs) noexcept {
        T op {};
        return op(lhs, rhs);
    }

    template < class T >
    struct ACallable {
        constexpr int operator()(int lhs, int rhs) noexcept {
            T op {};
            return op(lhs, rhs);
        }

        constexpr int Do(int lhs, int rhs) noexcept {
            T op {};
            return op(lhs, rhs);
        }
    };

    constexpr bool TestCtorAndCalls() {
        // function ptr
        {
            auto f1 = mr::function< int(int, int) > { Do< std::plus< int > > };
            CONSTEXPR_ASSERT(f1(1, 2) == 3);

            f1 = Do< std::minus< int > >;
            CONSTEXPR_ASSERT(f1(1, 2) == -1);

            decltype(f1) f2 = Do< std::multiplies< int > >;
            CONSTEXPR_ASSERT(f2(1, 2) == 2);
        }

        // function object
        {
            auto f1 = mr::function< int(int, int) > { ACallable< std::plus< int > > {} };
            CONSTEXPR_ASSERT(f1(1, 2) == 3);

            f1 = ACallable< std::minus< int > > {};
            CONSTEXPR_ASSERT(f1(1, 2) == -1);

            decltype(f1) f2 = ACallable< std::multiplies< int > > {};
            CONSTEXPR_ASSERT(f2(1, 2) == 2);
        }

        // lambda
        {
            auto f1 = mr::function< int(int, int) > { [](int lhs, int rhs) { return std::plus< int > {}(lhs, rhs); } };
            CONSTEXPR_ASSERT(f1(1, 2) == 3);

            f1 = [](int lhs, int rhs) { return std::minus< int > {}(lhs, rhs); };
            CONSTEXPR_ASSERT(f1(1, 2) == -1);

            decltype(f1) f2 = [](int lhs, int rhs) { return std::multiplies< int > {}(lhs, rhs); };
            CONSTEXPR_ASSERT(f2(1, 2) == 2);

            mr::function< int(int, int) > ff = [](int x, int y) { return x + y; };
            CONSTEXPR_ASSERT(ff(1, 2) == 3);

            int                      Value = 3;
            mr::function< int(int) > f3 =
                std::bind([](int x, int y) { return x - y; }, std::placeholders::_1, std::ref(Value));

            CONSTEXPR_ASSERT(f3(5) == 2);
            Value = 10;
            CONSTEXPR_ASSERT(f3(5) == -5);

            mr::function< int(const mr::any&, const mr::any&) > f4 = [](const mr::any& x, const mr::any& y) -> int {
                auto x_value = *mr::any_cast< int >(std::addressof(x));
                auto y_value = *mr::any_cast< int >(std::addressof(y));

                return x_value + y_value;
            };
            CONSTEXPR_ASSERT(f4(5, 5) == 10);
        }

        // member function
        {
            auto f1 =
                mr::function< int(ACallable< std::plus< int > >, int, int) > { &ACallable< std::plus< int > >::Do };
            CONSTEXPR_ASSERT(f1(ACallable< std::plus< int > > {}, 1, 2) == 3);

            ACallable< std::minus< int > > obj_minus {};
            auto f2 = mr::function< int(int, int) > { std::bind(&ACallable< std::minus< int > >::Do, obj_minus,
                                                                std::placeholders::_1, std::placeholders::_2) };
            CONSTEXPR_ASSERT(f2(1, 2) == -1);

            ACallable< std::multiplies< int > > obj_multiply {};
            auto f3 = mr::function< int(const mr::any&, int) > { [&](const mr::any& lhs, int rhs) -> int {
                return std::bind(&ACallable< std::multiplies< int > >::Do, obj_multiply, std::placeholders::_1,
                                 std::placeholders::_2)(mr::any_cast< int >(lhs), rhs);
            } };
            CONSTEXPR_ASSERT(f3(6, 3) == 18);
        }

        return true;
    }

    constexpr bool TestMove() {
        auto f1 = mr::function< int(int, int) > {};

        CONSTEXPR_ASSERT(!static_cast< bool >(f1))

        f1 = mr::function { Do< std::plus< int > > };
        CONSTEXPR_ASSERT(f1(1, 1) == 2)

        CONSTEXPR_ASSERT(*f1.target< int (*)(int, int) noexcept >() == Do< std::plus< int > >)
        CONSTEXPR_ASSERT(*f1.target< int (*)(int, int) noexcept >() != Do< std::minus< int > >)

        mr::function f2 { std::move(f1) };
        CONSTEXPR_ASSERT(!static_cast< bool >(f1))
        CONSTEXPR_ASSERT(static_cast< bool >(f2))

        return true;
    }

    constexpr bool TestGetTarget() {
        // function ptr
        {
            auto f1 = mr::function< int(int, int) > { Do< std::plus< int > > };

            auto tf1 = f1.target< int (*)(int, int) >();
            CONSTEXPR_ASSERT(tf1 == nullptr);

            auto tf2 = f1.target< int (*)(int, int) noexcept >();
            CONSTEXPR_ASSERT(*tf2 == Do< std::plus< int > >);

            auto nullf1 = f1.target< float (*)(int, int) >();
            CONSTEXPR_ASSERT(nullf1 == nullptr);
        }

        // function object
        {
            auto f1  = mr::function< int(int, int) > { ACallable< std::plus< int > > {} };
            auto tf1 = f1.target< ACallable< std::plus< int > > >();
            CONSTEXPR_ASSERT(tf1 != nullptr);

            f1       = ACallable< std::minus< int > > {};
            auto tf2 = f1.target< ACallable< std::minus< int > > >();
            CONSTEXPR_ASSERT(tf2 != nullptr);
        }

        // lambda
        {
            auto lambda = [](int lhs, int rhs) { return std::plus< int > {}(lhs, rhs); };

            auto f1  = mr::function< int(int, int) > { lambda };
            auto tf1 = f1.target< decltype(lambda) >();
            CONSTEXPR_ASSERT(tf1 != nullptr);
        }

        // member function
        {
            auto f1 =
                mr::function< int(ACallable< std::plus< int > >, int, int) > { &ACallable< std::plus< int > >::Do };
            auto tf1 = f1.target< decltype(&ACallable< std::plus< int > >::Do) >();
            CONSTEXPR_ASSERT(tf1 != nullptr);
        }
        return true;
    }

    constexpr bool TestSwap() {
        mr::function f1 = Do< std::plus< int > >;
        mr::function f2 = Do< std::minus< int > >;

        f1.swap(f2);

        CONSTEXPR_ASSERT(f1);
        CONSTEXPR_ASSERT(f2);

        CONSTEXPR_ASSERT(*f1.target< int (*)(int, int) noexcept >() == Do< std::minus< int > >)
        CONSTEXPR_ASSERT(*f2.target< int (*)(int, int) noexcept >() != Do< std::minus< int > >)

        return true;
    }

#ifndef _MSC_VER
    constexpr bool TestInitListOfFunctions() {
        {
            std::initializer_list< mr::function< int(int, int) > > lst = {
                mr::function< int(int, int) > { Do< std::plus< int > > },
                mr::function< int(int, int) > { Do< std::minus< int > > },
                mr::function< int(int, int) > { Do< std::multiplies< int > > }
            };

            CONSTEXPR_ASSERT((*lst.begin())(4, 3) == 7);
            return true;
        }
        return true;
    }
#endif // _MSC_VER

#ifdef __cpp_lib_constexpr_vector
    constexpr bool TestVectorOfFunctions() {
        {
            std::vector< mr::function< int(int, int) > > vec {};

            vec.push_back(Do< std::plus< int > >);
            vec.push_back(Do< std::minus< int > >);
            vec.push_back(Do< std::multiplies< int > >);

            CONSTEXPR_ASSERT(vec[1](4, 3) == 1);
            vec.clear();
        }
        return true;
    }
#endif // __cpp_lib_constexpr_vector

} // namespace test

static_assert(test::TestCtorAndCalls());
static_assert(test::TestGetTarget());
static_assert(test::TestMove());
static_assert(test::TestSwap());

#ifndef _MSC_VER
static_assert(test::TestInitListOfFunctions());
#endif // _MSC_VER

#ifdef __cpp_lib_constexpr_vector
static_assert(test::TestVectorOfFunctions());
#endif // __cpp_lib_constexpr_vector

int main() {
    assert(test::TestCtorAndCalls());
    assert(test::TestGetTarget());
    assert(test::TestMove());
    assert(test::TestSwap());

#ifndef _MSC_VER
    assert(test::TestInitListOfFunctions());
#endif // _MSC_VER

#ifdef __cpp_lib_constexpr_vector
    assert(test::TestVectorOfFunctions());
#endif // __cpp_lib_constexpr_vector
}
