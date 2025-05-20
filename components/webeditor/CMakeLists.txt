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
        ladderlib_esp32
        
    EMBED_TXTFILES
        ./ladder_editor.html
    
    EMBED_FILES
        ./webeditor_favicon.ico
)
