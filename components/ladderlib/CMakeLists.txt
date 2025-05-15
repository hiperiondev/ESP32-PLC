file(
    GLOB_RECURSE
        SOURCES
            ladderlib/source/*.c
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        ladderlib/source/include/
        ladderlib/
    REQUIRES
        json
)