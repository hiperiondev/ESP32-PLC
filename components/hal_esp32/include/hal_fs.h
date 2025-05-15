/*
 * Copyright 2025 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/ESP32-PLC *
 *
 * This is based on other projects, please contact their authors for more information.
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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
