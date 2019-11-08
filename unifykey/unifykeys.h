/*
 * vendor/amlogic/common/fota/include/unifykeys.h
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *
 */

#ifndef __UNIFYKEYS_H
#define __UNIFYKEYS_H

#include <unistd.h>
#include <sys/ioctl.h>

#define KEYUNIFY_ATTACH    _IO('f', 0x60)
#define KEYUNIFY_GET_INFO    _IO('f', 0x62)

#define UNIFYKEYS_PATH "/dev/unifykeys"
#define DEVICE_NAME "usid"
#define KEY_UNIFY_NAME_LEN (48)


struct key_item_info_t {
    unsigned int id;
    char name[KEY_UNIFY_NAME_LEN];
    unsigned int size;
    unsigned int permit;
    unsigned int flag;
    unsigned int reserve;
};

void dump_keyitem_info(struct key_item_info_t *info);
int init_unifykey(const char *path);
int get_info_unifykey(const char *path);
int test_read_key(const char * node, char *name,char* refbuf);
int test_write_key(const char * node, char *buff, char *name);
int read_seek_write_unifykey(const char *path);
int get_serial_number(char* readbuf);

#endif
