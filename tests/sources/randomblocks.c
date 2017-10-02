#include <stdio.h>

// obfuscated add 24
void foo(int *p) {
    int i = 0;
    for (; i < 32; ++i) {
        if ( i % 4 == 3 ) {
            *p += 3;
        }
    }
}

int main() {
    int p = 0;
    foo(&p);
    test_assert( p == 24 );
    while (p < 1000) {
        for (int i = 0; i < 10; ++i) {
            foo(&p);
            p += 4;
            foo(&p);
            test_assert( p == (24 + 52 * (i + 1)) );
        }
    }
    test_assert( p == 1064 );
}
