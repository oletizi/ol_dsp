#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstdio>

int main()
{
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(nullptr, &engine);
    if (result != MA_SUCCESS) {
        return -1;
    }

    ma_engine_play_sound(&engine, "/Users/orion/work/ol_dsp/workouts/drum_loop.wav", nullptr);

    printf("Press Enter to quit...");
    getchar();

    ma_engine_uninit(&engine);

    return 0;
}
