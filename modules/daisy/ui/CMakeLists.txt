add_library(ol_daisy_ui
        GpioPool.h
        Page.cpp
        PolyvoiceInputs.h
        ui.h
)
target_link_libraries(ol_daisy_ui daisy ol_synthlib)
target_include_directories(ol_daisy_ui PUBLIC .)
