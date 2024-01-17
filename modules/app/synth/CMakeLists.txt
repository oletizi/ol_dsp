add_library(ol_synth_app ol_app_synth.h)
target_link_libraries(ol_synth_app spline ol_corelib ol_ctllib ol_fxlib ol_guilib ol_synthlib)
target_include_directories(ol_synth_app INTERFACE ../..)
set_target_properties(ol_synth_app PROPERTIES LINKER_LANGUAGE CXX)