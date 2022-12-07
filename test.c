#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    for ( int i = 0; i < 5000; i++)
        printf("%d\n", clock());
}