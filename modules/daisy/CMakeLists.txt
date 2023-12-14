add_library(ol_daisy_ui
        ui/Page.cpp
        ui/ui.h
)

add_library(daisy daisy_dummy.h
        daisy_dummy.cpp)
target_include_directories(ol_daisy_ui PUBLIC . ../../libs/libDaisy/src)


add_executable(ol_daisy_multifx fx/multifx/main.cpp)
target_link_libraries(ol_daisy_multifx daisy DaisySP ol_daisy_ui ol_corelib ol_fxlib ol_synthlib)
target_compile_definitions(ol_daisy_multifx PRIVATE DAISY_NATIVE)

#add_executable(ol_daisy_workouts_sandbox workouts/sandbox/main.cpp)
#target_link_libraries(ol_daisy_workouts_sandbox daisy DaisySP ol_daisy_ui ol_corelib ol_fxlib ol_synthlib)
#target_compile_definitions(ol_daisy_workouts_sandbox PRIVATE DAISY_NATIVE)


add_executable(ol_daisy_sampler sampler/main.cpp)
target_link_libraries(ol_daisy_sampler daisy DaisySP ol_daisy_ui ol_corelib ol_fxlib ol_synthlib)
target_compile_definitions(ol_daisy_sampler PRIVATE DAISY_NATIVE)

