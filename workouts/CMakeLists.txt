# Workout buddy
add_library(workout-buddy workout_buddy.cpp)
target_link_libraries(workout-buddy ol_corelib ol_fxlib ol_synthlib rtmidi)
target_include_directories(workout-buddy PUBLIC ../libs/rtmidi ../libs/miniaudio)

# Basic audio file player
add_executable(player-workout player_main.cpp)
target_include_directories(player-workout PUBLIC . ../libs/miniaudio)

# Miniaudio workout: exercises Miniaudio functions
add_executable(miniaudio-main miniaudio_main.cpp)
target_include_directories(miniaudio-main PUBLIC . ../libs/miniaudio)

# Sample workout: exercises the Sample interface
add_executable(sample-workout sample_main.cpp)
target_link_libraries(sample-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP workout-buddy)
target_include_directories(sample-workout PUBLIC . ../libs/miniaudio)

