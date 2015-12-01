
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint32_t left[16] __attribute__((aligned(64))) = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint32_t right[16] __attribute__((aligned(64))) = {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500};
uint32_t result[16] __attribute__((aligned(64))) = {0};

int main(int argc, char** argv) {
    // leftとrightをいっぺんに加算する
    asm (
        "vmovdqa32 (%0), %%zmm0;"
        "vpaddd (%1), %%zmm0, %%zmm0;"
        "vmovdqa32 %%zmm0, (%2);"
        ::"a"(left), "b"(right), "c"(result));

    // 結果を出力
    for (int i = 0; i < 16; i++) {
        printf("result[%d] = %d\n", i, result[i]);
    }
    return 0;
}
