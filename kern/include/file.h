#ifndef _FILE_H_
#define _FILE_H_

#include <lib.h>
#include <device.h>
#include <uio.h>
#include <fs.h>
#include <vnode.h>
#include <thread.h>

#define MAX_FILENAME_LEN 1024

struct file_desc{
    char name[MAX_FILENAME_LEN];
    struct vnode* vn;
    int flags;
    off_t offset;
    int ref_count;
    struct lock* lock;
};


//int fd_table_init(struct thread *thr);
int fd_table_init(struct thread *thr);
int sys_open(userptr_t filename, int flags, int mode, int *retval);
int sys_close(int fd, int *retval);
int sys_read(int fd, userptr_t buf, size_t buflen, int *retval);
int sys_write(int fd, userptr_t buf, size_t nbytes, int *retval);
int sys_lseek(int fd, off_t pos, int whence, off_t *retval);

#endif
