add_library(ol_daisy_io ol_daisy_io.h)
target_link_libraries(ol_daisy_io daisy ryml ol_iolib ol_synthlib)
target_include_directories(ol_daisy_io PUBLIC ../..)
