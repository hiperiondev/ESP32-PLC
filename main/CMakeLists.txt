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
        vfs
        ladderlib
        ladderlib_esp32
        console
        driver
        ftpserver
        nvs_flash
        hal_esp32
        cmd_system
        cmd_ladderlib
        wifi-provisioning
        webeditor
        cron
)