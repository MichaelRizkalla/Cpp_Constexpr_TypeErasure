#include <cassert>
#include <constexpr_any.h>
#include <constexpr_function.h>
#include <functional>
#include <vector>

#define ASSERT(x)     \
    if (!(x)) {       \
        return false; \
    }

namespace test {

    template < class T >
    constexpr int Do(int lhs, int rhs) {
        T op {};
        return op(lhs, rhs);
    }

    constexpr bool TestCtorAndCalls() {
        auto f1 = mr::function< int(int, int) > { Do< std::plus< int > > };
        ASSERT(f1(1, 2) == 3);

        f1 = Do< std::minus< int > >;
        ASSERT(f1(1, 2) == -1);

        decltype(f1) f2 = Do< std::multiplies< int > >;
        ASSERT(f2(1, 2) == 2);

        mr::function< int(int, int) > ff = [](int x, int y) { return x + y; };
        ASSERT(ff(1, 2) == 3);

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

            ASSERT((*lst.begin())(4, 3) == 7);
            return true;
        }
        return true;
    }
#endif // _MSC_VER

    constexpr bool TestGetTarget() {
        auto f1 = mr::function< int(int, int) > { Do< std::plus< int > > };

        auto tf1 = f1.target< int (*)(int, int) >();

        ASSERT((*tf1)(1, 2) == 3);
        ASSERT(*tf1 == Do< std::plus< int > >);

        auto nullf1 = f1.target< float (*)(int, int) >();
        ASSERT(nullf1 == nullptr);

        return true;
    }

#ifdef __cpp_lib_constexpr_vector
    constexpr bool TestVectorOfFunctions() {
        {
            std::vector< mr::function< int(int, int) > > vec {};

            vec.push_back(Do< std::plus< int > >);
            vec.push_back(Do< std::minus< int > >);
            vec.push_back(Do< std::multiplies< int > >);

            ASSERT(vec[1](4, 3) == 1);
            vec.clear();
        }
        return true;
    }
#endif // __cpp_lib_constexpr_vector

} // namespace test

static_assert(test::TestCtorAndCalls());
static_assert(test::TestGetTarget());

#ifndef _MSC_VER
static_assert(test::TestInitListOfFunctions());
#endif // _MSC_VER

#ifdef __cpp_lib_constexpr_vector
// static_assert(test::TestVectorOfFunctions());
#endif // __cpp_lib_constexpr_vector

int main() {
}
