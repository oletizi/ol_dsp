add_library(ol_iolib
        ol_iolib.h
        PatchLoader.cpp
        PatchLoader.h
        Serializer.h
        Serial.h
)
target_link_libraries(ol_iolib ryml ol_corelib ol_synthlib)
target_include_directories(ol_iolib PUBLIC .)