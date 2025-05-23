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
        driver
        ladderlib
        hal_esp32
        esp_timer
        webeditor
)