#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int n;
    // scanf("%d", &n);
    int s = 0;  // Initialize s
                // 2+4+6.....+n
    for (int i = 2; i <= n; i = i + 2) {
        s += i;
    }
    printf("%d\n", s);
    return 0;
}