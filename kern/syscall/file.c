/*
	File system calls
*/

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <array.h>
#include <spl.h>
#include <thread.h>
#include <current.h>
#include <vfs.h>
#include <synch.h>
#include <mainbus.h>
#include <vnode.h>
#include <file.h>
#include <copyinout.h>
#include <kern/stat.h>
#include <kern/seek.h>


int fd_table_init(struct thread *thr)
{
	int result;
	char fname[]="con:";
	char fname1[]="con:";
	char fname2[]="con:";
	struct vnode *v0;
	struct vnode *v1;
	struct vnode *v2;

	if(thr->fd_table[0] == NULL)
	{
		thr->fd_table[0] = (struct file_desc *)kmalloc(sizeof(struct file_desc));
		result = vfs_open(fname, O_RDONLY, 0664, &v0);
		if(result)
		{
			kprintf("file table initialization failed\n");
			return EINVAL;
		}
		strcpy(thr->fd_table[0]->name,fname);
		thr->fd_table[0]->vn = v0;
		thr->fd_table[0]->flags = O_RDONLY;
		thr->fd_table[0]->offset = 0;
		thr->fd_table[0]->lock = lock_create(fname);
		thr->fd_table[0]->ref_count=1;
	}

	if(thr->fd_table[1] == NULL)
	{
		thr->fd_table[1] = (struct file_desc *)kmalloc(sizeof(struct file_desc));
		result = vfs_open(fname1, O_WRONLY, 0664, &v1);
		if(result)
		{
			kprintf("file table initialization failed\n");
			return EINVAL;
		}
		strcpy(thr->fd_table[1]->name,fname);
		thr->fd_table[1]->vn = v1;
		thr->fd_table[1]->flags = O_WRONLY;
		thr->fd_table[1]->offset = 0;
		thr->fd_table[1]->lock = lock_create(fname);
		thr->fd_table[1]->ref_count=1;
	}

	if(thr->fd_table[2] == NULL)
	{
		thr->fd_table[2] = (struct file_desc *)kmalloc(sizeof(struct file_desc));
		result = vfs_open(fname2, O_WRONLY, 0664, &v2);
		if(result)
		{
			kprintf("file table initialization failed2\n");
			return EINVAL;
		}
		strcpy(thr->fd_table[2]->name,fname);
		thr->fd_table[2]->vn = v2;
		thr->fd_table[2]->flags = O_WRONLY;
		thr->fd_table[2]->offset = 0;
		thr->fd_table[2]->lock = lock_create(fname);
		thr->fd_table[2]->ref_count=1;
	}

	return 0;

}

int sys_open(userptr_t filename, int flags, int mode, int *retval)
{
	int result, i;
	struct vnode *v;
	size_t siz;
	char *fname = (char *)kmalloc(sizeof(char)*PATH_MAX);

	if(!(flags&O_RDONLY || flags&O_WRONLY || flags&O_RDWR))
		return EINVAL;

	if(filename==NULL)
		return EFAULT;

	result=copyinstr(filename, fname, PATH_MAX, &siz);
	if(result)
		return result;

	//Check for modes remaining.

	for(i=3;i<OPEN_MAX;i++)
	{
		if(curthread->fd_table[i]==NULL)
			break;
	}
	if(i==OPEN_MAX)
		return ENFILE;

	curthread->fd_table[i] = (struct file_desc *)kmalloc(sizeof(struct file_desc));

	if(curthread->fd_table[i] == NULL)
	{
		return ENFILE;
	}

	result = vfs_open(fname, flags, mode, &v);
	//kprintf("vfs_open result: %d %d",result,flags);
	if(result)
		return result;

	strcpy(curthread->fd_table[i]->name,fname);
	curthread->fd_table[i]->vn = v;
	curthread->fd_table[i]->flags = flags;
	curthread->fd_table[i]->offset = 0;
	curthread->fd_table[i]->lock = lock_create(fname);
	curthread->fd_table[i]->ref_count=1;

	kfree(fname);

	*retval=i;

	return 0;

}

int sys_close(int fd, int *retval)
{
	if(fd<3 || fd>OPEN_MAX)
	{
		//*retval=EBADF;
		return EBADF;
	}
	if(curthread->fd_table[fd]==NULL)
	{
		//*retval=EBADF;
		return EBADF;
	}

	lock_acquire(curthread->fd_table[fd]->lock);
	vfs_close(curthread->fd_table[fd]->vn);
	if(curthread->fd_table[fd]->ref_count==1)
	{
		lock_release(curthread->fd_table[fd]->lock);
		lock_destroy(curthread->fd_table[fd]->lock);
		kfree(curthread->fd_table[fd]);
		curthread->fd_table[fd]=NULL;
	}
	else
	{
		KASSERT(curthread->fd_table[fd]->ref_count > 1);
		curthread->fd_table[fd]->ref_count--;
		lock_release(curthread->fd_table[fd]->lock);
	}
	*retval=0;
	return 0;
}

int sys_read(int fd, userptr_t buf, size_t buflen, int *retval)
{
	int result,mode;
	struct iovec iov;
	struct uio u_io;
	size_t lenread;

	if(fd<1 || fd>OPEN_MAX)
	{
		//*retval=EBADF;
		return EBADF;
	}
	if(curthread->fd_table[fd]==NULL)
	{
		//*retval=EBADF;
		return EBADF;
	}

	lock_acquire(curthread->fd_table[fd]->lock);

	//void *kbuff=kmalloc(sizeof(buflen));

	mode=curthread->fd_table[fd]->flags & O_ACCMODE;
	if(mode == O_WRONLY)
	{
		lock_release(curthread->fd_table[fd]->lock);
		return EBADF;
	}

	//uio_kinit(struct iovec *iov, struct uio *u, void *kbuf, size_t len, off_t pos, enum uio_rw rw)
	uio_kinit(&iov, &u_io, buf, buflen, curthread->fd_table[fd]->offset, UIO_READ);
	result = VOP_READ(curthread->fd_table[fd]->vn, &u_io);
	if (result) {
		lock_release(curthread->fd_table[fd]->lock);
		return result;
	}

	//copyout(kbuff,buf,buflen);
	//char *c=(char*)kbuff;
	//kprintf("%c",*c);

	curthread->fd_table[fd]->offset = u_io.uio_offset;
	lenread = buflen - u_io.uio_resid;

	lock_release(curthread->fd_table[fd]->lock);

	*retval=lenread;
	return 0;
}

int sys_write(int fd, userptr_t buf, size_t nbytes, int *retval)
{
	int result;
	struct iovec iov;
	struct uio u_io;
	//size_t siz;
	size_t lenread;

	if(fd<1 || fd>OPEN_MAX)
	{
		//*retval=EBADF;
		return EBADF;
	}
	if(curthread->fd_table[fd]==NULL)
	{
		//*retval=EBADF;
		return EBADF;
	}

	lock_acquire(curthread->fd_table[fd]->lock);

	//Not opened in RW or write mode.
	if(!(curthread->fd_table[fd]->flags & O_WRONLY || curthread->fd_table[fd]->flags & O_RDWR))
	{
		lock_release(curthread->fd_table[fd]->lock);
		return EBADF;
	}

	/*char *kbuff = (char *)kmalloc(nbytes);
	copyinstr((userptr_t)buf, kbuff, strlen(kbuff), &siz);*/
	//char *c=kbuff;
	//kprintf("%c",*c);

	uio_kinit(&iov, &u_io, buf, nbytes, curthread->fd_table[fd]->offset, UIO_WRITE);
	result = VOP_WRITE(curthread->fd_table[fd]->vn, &u_io);
	if (result)
	{
		lock_release(curthread->fd_table[fd]->lock);
		return result;
	}

	curthread->fd_table[fd]->offset = u_io.uio_offset;
	lenread = nbytes - u_io.uio_resid;

	lock_release(curthread->fd_table[fd]->lock);
	//kprintf("\n\nhii\n\n");
	//result=lenread;
	*retval=lenread;
	return 0;
}

int sys_lseek(int fd, off_t pos, int whence, off_t *retval)
{
	int result;
	off_t end;
	struct stat statptr;

	//kprintf("target: %llu",pos);

	if(fd<1 || fd>OPEN_MAX)
	{
		//*retval=EBADF;
		return EBADF;
	}
	if(curthread->fd_table[fd]==NULL)
	{
		//*retval=EBADF;
		return EBADF;
	}

	lock_acquire(curthread->fd_table[fd]->lock);

    if(pos<0)
    {
    	lock_release(curthread->fd_table[fd]->lock);
    	return EINVAL;
    }

    if(!(whence==SEEK_SET || whence==SEEK_CUR || whence==SEEK_END))
    {
    	lock_release(curthread->fd_table[fd]->lock);
    	return EINVAL;
    }

	if(whence==SEEK_CUR)
	{
		pos = curthread->fd_table[fd]->offset + pos;
	}
	else if (whence==SEEK_END)
	{
		VOP_STAT(curthread->fd_table[fd]->vn, &statptr);
    	end = statptr.st_size;
		pos = pos + end;
	}
	result = VOP_TRYSEEK(curthread->fd_table[fd]->vn, pos);
	if (result) {
		lock_release(curthread->fd_table[fd]->lock);
		return result;
	}

	curthread->fd_table[fd]->offset = pos;

	lock_release(curthread->fd_table[fd]->lock);

	//kprintf("final: %llu\n",pos);
	*retval=pos;
	return 0;
}

/*
int sys__getcwd(char *buf,size_t buflen,int *retval)
{
//  int vfs_getcwd(struct uio *buf);
   
  // Hence need to initialize a uio structure

  struct iovec iov;
  struct uio ku;
  size_t size;
  
  char *filename = (char *)kmalloc(buflen);

  // need to call kinit and then store result of getcwd into an int variable
  // need to call kinit
  uio_kinit(&iov,&ku,&filename,buflen-1,0,UIO_READ); 
  int result = vfs_getcwd(&ku);
  if(result)
  {
  // kprintf("vfs_getcwd failed (%s) \n",stderror(result));
   return result;
  }
  
  // references taken from menu.c

    // null terminate 
   filename[buflen-1-ku.uio_resid] = 0;

   //need to copy the kernel buffer to the user buffer else there are compile problems
   copyoutstr(filename,(userptr_t)buf,buflen,&size);
  *retval = buflen-1-ku.uio_resid;
   result = buflen-1-ku.uio_resid;
   kfree(filename);
   return result;
   
}
// check and change
int sys_chdir(const char *pathname,int *retval)
{
     // need to use vfs_chdir
     // before that need to setup 
   int result;
   size_t size;

    // no other process should acces the file while it is having its path changed
   int a = splhigh();
    // need to make a copy of the pathname and copy it to a new kernel buffer 
    // using copyinstr
   char *path = (char *)kmalloc(sizeof(char)*PATH_MAX);
   copyinstr((const_userptr_t)pathname,path,PATH_MAX,&size);
    // the pathname now is in the kernel buffer
      
   result = vfs_chdir(path); 
   if(result)
    {
      splx(a);
      return result;
    }
     
    // success condition for pathname : it has been changed

  *retval = 0;
  kfree(path);
  splx(a);
  return 0;
}

*/
