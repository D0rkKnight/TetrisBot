#include <stdio.h>
#include <stdlib.h>

int main() {
    int *arr = malloc(8); // malloc allocates dirty memory
    *arr = 0;

    char c = *arr + '0';
    char b = 'x';

    printf("Strings and stuff %c\n", c);
}