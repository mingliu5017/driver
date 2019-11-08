/*
 * vendor/amlogic/common/fota/samples/unifykey_example_file_ops.c
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <linux/ioctl.h>

#include "unifykeys.h"

int init_unifykey(const char *path)
{
    int fp;
    int ret;
    struct key_item_info_t key_item_info;

    fp  = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -1;
    }
    strcpy(key_item_info.name, DEVICE_NAME);

    ret =ioctl(fp, KEYUNIFY_ATTACH, &key_item_info);
    close(fp);
    return ret;
}

int get_info_unifykey(const char *path)
{
    int fp;
    int ret;
    struct key_item_info_t key_item_info;

    memset(&key_item_info, 0x00, sizeof(key_item_info));
    fp  = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -1;
    }
    key_item_info.id = 0;

    ret =ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    close(fp);
    dump_keyitem_info(&key_item_info);

    return ret;
}

int read_seek_write_unifykey(const char *path)
{
    int fp;
    int ret;
    char tmp_buf[6];

    fp = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -1;
    }

    ret = read(fp, tmp_buf, 5);
    if (ret < 0) {
        printf("%s: read err\n", __func__);
        return -2;
    }
    tmp_buf[5] = 0x00;
    printf("org read: %s\n", tmp_buf);

    lseek(fp, 1, SEEK_SET);
    ret = read(fp, tmp_buf, 5);
    if (ret < 0) {
        printf("%s: read err\n", __func__);
        return -3;
    }
    tmp_buf[5] = 0x00;
    printf("seek read: %s\n", tmp_buf);
    close(fp);

    fp  = open(path, O_RDWR);
    ret = write(fp, "abc12", 5);
    if (ret < 0) {
        printf("%s: read err\n", __func__);
        return -4;
    }
    close(fp);
    return 0;
}

void dump_mem(unsigned char * buffer, int count)
{
    int i;

    if (NULL == buffer || count == 0) {
        printf("%s() %d: %p, %d", __func__, __LINE__, buffer, count);
        return;
    }
    for (i=0; i<count ; i++) {
        if (i % 16 == 0)
            printf("dump data: \n");
        printf("%02x ", buffer[i]);
    }
    printf("\n");
}

void dump_keyitem_info(struct key_item_info_t *info)
{
    if (info == NULL)
        return;
    printf("id:	%d\n", info->id);
    printf("name:	%s\n", info->name);
    printf("size:	%d\n", info->size);
    printf("permit:	0x%x\n", info->permit);
    printf("flag:	0x%x\n", info->flag);
    return;
}

int test_read_key(const char * node, char *name,char* refbuf)
{
    int ret = 0;
    char buffer[4096] = {0};
    unsigned long ppos;
    size_t readsize;
    int fp;
    struct key_item_info_t key_item_info;

    if ((NULL == node) || (NULL == name)) {
        printf("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }

    fp  = open(node, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", node);
        return -2;
    }

    /* seek the key index need operate. */
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    ppos = key_item_info.id;
    lseek(fp, ppos, SEEK_SET);
    if (ret < 0)
        goto error;

    dump_keyitem_info(&key_item_info);
    if (key_item_info.flag) {
        ret = read(fp, buffer, key_item_info.size);
        if (ret < 0)
             goto error;
        dump_mem(buffer, ret);
        strcpy(refbuf,buffer);
    }
error:
    close(fp);
    return ret;
}


int test_write_key(const char * node, char *buff, char *name)
{
    int ret = 0;
    char buffer[4096] = {0};
    unsigned long ppos;
    size_t readsize, writesize;
    int fp;
    struct key_item_info_t key_item_info;

    if ((NULL == node) || (NULL == buff) || (NULL == name)) {
        printf("%s() %d: invalid param!\n", __func__, __LINE__);
        return -1;
    }

    fp = open(node, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", node);
       return -1;
    }

    /* seek the key index need operate. */
    strcpy(key_item_info.name, name);
    ret = ioctl(fp, KEYUNIFY_GET_INFO, &key_item_info);
    if (ret < 0)
        goto _out;
    ppos = key_item_info.id;
    lseek(fp, ppos, SEEK_SET);

    dump_keyitem_info(&key_item_info);
    writesize = write(fp, buff, strlen(buff));
    if (writesize != strlen(buff)) {
        printf("%s() %d: write %s failed!\n", __func__, __LINE__,
            key_item_info.name);
    }
_out:
    close(fp);
    return ret;
}





