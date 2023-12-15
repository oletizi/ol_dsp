add_library(ol_corelib ol_corelib.h)
target_include_directories(ol_corelib PUBLIC .. .)
set_target_properties(ol_corelib PROPERTIES LINKER_LANGUAGE CXX)