add_executable(player-workout player_main.cpp)
#target_link_libraries(player-workout ol_corelib ol_fxlib ol_synthlib rtmidi DaisySP)
target_include_directories(player-workout PUBLIC . ../libs/miniaudio)
