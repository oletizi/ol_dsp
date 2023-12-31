# 1.57
add_executable(ol_teensy_workouts_uart uart/loop.h
        uart/loop.cpp)
target_link_libraries(ol_teensy_workouts_uart ol_teensy)
set_target_properties(ol_teensy_workouts_uart PROPERTIES LINKER_LANGUAGE CXX)