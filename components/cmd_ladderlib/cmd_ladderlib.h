/*
 * cmd_ladderlib.h
 *
 *  Created on: 25 feb 2025
 *      Author: egonzalez
 */

#ifndef CMD_LADDERLIB_H_
#define CMD_LADDERLIB_H_

void register_ladder_status(void);
void register_fs_ls(void);
void register_fs_rm(void);
void register_ladder_save(void);
void register_ladder_load(void);
void register_ladder_start(void);
void register_ladder_stop(void);
void register_ftpserver(void);
void register_port_test(void);

#endif /* CMD_LADDERLIB_H_ */
