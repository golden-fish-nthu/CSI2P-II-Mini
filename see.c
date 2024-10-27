#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int x, y, z;
    x = 2;
    y = 3;
    z = 5;

    y + 5 * x - 2 + z * 3;
    printf("%d %d %d\n", x, y, z);
    x = 5;
    printf("%d %d %d\n", x, y, z);
    y = 6;
    printf("%d %d %d\n", x, y, z);
    x = (3 + 5) - 8 * (10 / 2);
    printf("%d %d %d\n", x, y, z);
    y = x * x - (12 * 12);
    printf("%d %d %d\n", x, y, z);
    z = z / z + (+-+-+-+-z - z) + (x * z) % z + (y + z) * 0 - x * y;
    printf("%d %d %d\n", x, y, z);
    x = (-y * -y - (y * y - 4 * x * z)) / (2 * x * 2 * x);
    printf("%d %d %d\n", x, y, z);
    //-880 880 28161
    return 0;
}
/*2 3 5
5 3 5
5 6 5
-32 6 5
-32 880 5
-32 880 28161
-880 880 28161*/