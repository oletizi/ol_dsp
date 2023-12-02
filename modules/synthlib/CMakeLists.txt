add_library(ol_synthlib Voice.cpp)
target_link_libraries(ol_synthlib ol_corelib ol_ctllib DaisySP)
target_include_directories(ol_synthlib PUBLIC .)