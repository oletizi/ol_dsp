add_library(ol_guilib guilib.h)
target_include_directories(ol_guilib PUBLIC ..)
set_target_properties(ol_guilib PROPERTIES LINKER_LANGUAGE CXX)
