add_library(ol_guilib INTERFACE guilib.h)
target_link_libraries(ol_guilib INTERFACE ol_corelib)
target_include_directories(ol_guilib INTERFACE ..)
set_target_properties(ol_guilib PROPERTIES LINKER_LANGUAGE CXX)
