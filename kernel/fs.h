#pragma once

#include "../common.h"

#define FILES_MAX 2
#define DISK_MAX_SIZE align_up(sizeof(struct file) * FILES_MAX, SECTOR_SIZE)

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char linknamne[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
    char data[];        // Flexible array member pointing to the start of file data.
} __attribute__((packed));

struct file {
    bool in_use;        // Whether the file is in use
    char name[100];     // File name
    char data[1024];    // File data
    size_t size;        // File size
};

void fs_init(void);
void fs_flush(void);
struct file *fs_lookup(const char *filename);
