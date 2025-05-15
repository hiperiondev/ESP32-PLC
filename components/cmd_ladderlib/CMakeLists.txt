file(
    GLOB_RECURSE
        SOURCES
            ./*.c
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        ./
    REQUIRES
        ladderlib
        ladderlib_esp32
        console  
        hal_esp32
        ftpserver   
)