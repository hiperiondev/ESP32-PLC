/*
 * @file hal_fs.h
 * @brief FILESYSTEM port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see
 * @note This is based on other projects. See license files
 */

#ifndef HAL_FS_H_
#define HAL_FS_H_

#include <stdio.h>

#define MOUNT_POINT           "/littlefs"
#define PARTITION_LABEL       "littlefs"

#define fs_init()             littlefs_init()
#define fs_open(FN, OT)       littlefs_fopen(FN, OT)
#define fs_reopen(FN, OT, ST) littlefs_freopen(FN, OT, ST)
#define fs_remove(FN)         littlefs_remove(FN)
#define fs_rename(ON, NN)     littlefs_rename(ON, NN)
#define fs_ls()               littlefs_ls()

  int littlefs_init(void);
 void littlefs_deinit(void);
FILE* littlefs_fopen(const char *file, const char *mode);
FILE* littlefs_freopen(const char *filename, const char *opentype, FILE *stream);
  int littlefs_test(char *file);
  int littlefs_remove(const char *file);
  int littlefs_rename(const char *file, char *newname);
  int littlefs_ls(void);

#endif /* HAL_FS_H_ */
