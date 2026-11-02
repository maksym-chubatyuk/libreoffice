/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include "file_image.hxx"
#include "pagein.hxx"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef LINUX
#include <sys/sysmacros.h>
#endif

/* do_pagein */
static void do_pagein (const char * filename)
{
    int result;
    file_image image = FILE_IMAGE_INITIALIZER;

    if (file_image_open (&image, filename) != 0)
        return;

    if ((result = file_image_pagein (&image)) != 0)
    {
        fprintf (stderr, "file_image_pagein %s: %s\n", filename, strerror(result));
    }

    file_image_close (&image);
}

#ifdef LINUX
/* Filesystems like btrfs report an anonymous st_dev with major 0, not the block device they live
   on: look up the source of the mount containing path instead. */
static bool getMountSourceDevice(char const * path, dev_t * pDevice)
{
    FILE * fp = fopen("/proc/self/mountinfo", "r");
    if (fp == nullptr)
        return false;
    char line[4096];
    size_t bestLen = 0;
    bool found = false;
    while (fgets(line, sizeof(line), fp) != nullptr)
    {
        /* "id parent major:minor root mountpoint options... - fstype source superoptions" */
        char mountPoint[2048];
        if (sscanf(line, "%*s %*s %*s %*s %2047s", mountPoint) != 1)
            continue;
        size_t const len = strlen(mountPoint);
        if (strncmp(path, mountPoint, len) != 0 || len < bestLen)
            continue;
        /* "/mnt/foo" must not match "/mnt/foobar" */
        if (len > 1 && path[len] != '/' && path[len] != '\0')
            continue;
        char const * separator = strstr(line, " - ");
        char source[2048];
        if (separator == nullptr || sscanf(separator, " - %*s %2047s", source) != 1)
            continue;
        bestLen = len;
        struct stat sourceStat;
        found = stat(source, &sourceStat) == 0 && S_ISBLK(sourceStat.st_mode);
        if (found)
            *pDevice = sourceStat.st_rdev;
    }
    fclose(fp);
    return found;
}
#endif

static bool isRotational(char const * path)
{
    bool ret = true;
#ifdef LINUX
    FILE * fp = nullptr;
    char fullpath[4096];
    struct stat out;
    char type[2];
    if (stat(path , &out) == -1)
        return ret;
    dev_t device = out.st_dev;
    if (major(device) == 0 && !getMountSourceDevice(path, &device))
        return ret;
    /* a whole device has a queue of its own, while a partition is a subdirectory of its device */
    sprintf(fullpath, "/sys/dev/block/%u:%u/queue/rotational",
            static_cast<unsigned>(major(device)), static_cast<unsigned>(minor(device)));
    if ((fp = fopen(fullpath, "r")) == nullptr)
    {
        sprintf(fullpath, "/sys/dev/block/%u:%u/../queue/rotational",
                static_cast<unsigned>(major(device)), static_cast<unsigned>(minor(device)));
        if ((fp = fopen(fullpath, "r")) == nullptr)
            return ret;
    }
    if (fgets(type, sizeof(type), fp))
        ret = type[0] == '1';
    fclose(fp);
#endif
    return ret;
}

void pagein_execute(char const * path, char const * file)
{
    char fullpath[4096];
    char *p = nullptr;
    FILE   * fp = nullptr;
    if(!isRotational(path))
        return;
    memset(fullpath, 0, sizeof(fullpath));
    strncpy (fullpath, path, 3000);
    if (!(p = strrchr (fullpath, '/')))
        p = fullpath;
    else
        p++;
    strncpy(p, file, 1024);
    p[strlen(p)] = '\0';
    if ((fp = fopen (fullpath, "r")) == nullptr)
    {

        fprintf (stderr, "fopen %s: %s\n", fullpath, strerror(errno));
        return;
    }
    while (fgets (p, 1024, fp) != nullptr)
    {
        p[strlen(p) - 1] = '\0';

        /* paths relative to the location of the pagein file */
        do_pagein (fullpath);
    }
    fclose (fp);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
