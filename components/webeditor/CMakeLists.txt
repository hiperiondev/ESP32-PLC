file(
    GLOB_RECURSE
        SOURCES
            ./*.c
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        .
    REQUIRES
        ladderlib
        esp_http_server
        
    EMBED_FILES
        ladder_editor.html
)
