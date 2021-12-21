#include <constexpr_function.h>

constexpr int sum(int x, int y) {
    return x + y;
}

constexpr int sub(int x, int y) {
    return x - y;
}

constexpr int constexpr_main() {
    using namespace mr;

    constexpr_function< int(int, int) > fsum1 { sum };
    constexpr_function< int(int, int) > fsub2 { sub };

    auto r1 = fsum1(1, 2); // 3
    auto r2 = fsub2(1, 2); // -1

    constexpr_function< int(int, int) > fsum3 { nullptr };
    fsum3.swap(fsum1);

    auto r3 = fsum3(1, 2); // 3

    // auto error = fsum1(1, 2);

    constexpr_function< int(int, int) > fsum4 = fsum3;

    auto r4 = fsum4(1, 2); // 3

    constexpr_function fsub5 = std::move(fsub2);

    auto r5 = fsub5(1, 2); // -1

    // auto error = fsub2(1, 2);

    fsum4 = fsub2; // copy empty function

    // auto error = fsum4(1, 2);

    fsum4 = sum;

    auto r6 = fsum4(1, 2);

    return r1 + r2 + r3 + r4 + r5 + r6; // 10
}

int main() {
    constexpr_main();
    static_assert(10 == constexpr_main());
}
