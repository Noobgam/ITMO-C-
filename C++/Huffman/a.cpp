#include <iostream>

struct bit {
    int len;
    template <bool a>
    void Add();

    template<>
    void Add<true>() {
        len <<= 1;
        len |= 1;
    }

    template<>
    void Add<false>() {
        len <<= 1;
    }

    void add(bool a) {
        if (a == 1)
            Add<true>();
        else
            Add<false>();
    }
};

int main() {
    bit a;
    a.add(1);
    a.add(0);
    return 0;
}