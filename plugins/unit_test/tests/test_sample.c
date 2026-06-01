#include <stdio.h>
#include <string.h>

static int passed = 0, failed = 0;

void assert_int(int got, int expected, const char *name) {
    if (got == expected) {
        printf("PASS: %s\n", name);
        passed++;
    } else {
        printf("FAIL: %s — expected %d got %d\n", name, expected, got);
        failed++;
    }
}

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

int main(void) {
    assert_int(add(1, 1),       2,  "add_basic");
    assert_int(add(-3, 3),      0,  "add_negative");
    assert_int(multiply(3, 4),  12, "multiply");
    assert_int(strlen("Qt6"),   3,  "strlen");

    /* Intentional failure */
    assert_int(42, 99, "intentional_failure");

    printf("\nResults: %d passed, %d failed.\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
