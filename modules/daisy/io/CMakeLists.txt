add_library(ol_daisy_io
        GpioPool.h
        PolyvoiceControls.h
        io.h
)
target_link_libraries(ol_daisy_io daisy ol_synthlib)
target_include_directories(ol_daisy_io PUBLIC .)
