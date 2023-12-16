//
// Created by Orion Letizi on 12/15/23.
//
#include <cstdio>
#include <stmlib/stmlib.h>
#include <stmlib/dsp/dsp.h>

int main() {

    while (true) {
        printf("Say hello: h\n");
        printf("command: ");
        while (auto c = getchar()) {
            if (c == 'h') {
                printf("Hello!\n");
            }
        }
    }
}