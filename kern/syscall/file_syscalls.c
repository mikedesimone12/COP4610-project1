/*
 * File-related system call implementations.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>

/*
 * open() - get the path with copyinstr, then use openfile_open and
 * filetable_place to do the real work.
 */
int
sys_open(const_userptr_t upath, int flags, mode_t mode, int *retval)
{
	const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;

	char *kpath = (char*)kmalloc(sizeof(char)*PATH_MAX);
	struct openfile *file;
	int result = 0;

	// check invalid flags
	if (allflags ==  flags)
		return EINVAL;

	// copy in supplied pathname
	result = copyinstr(upath, kpath, PATH_MAX, NULL);
	if(result)
		return EFAULT;
	// open the file
	result = openfile_open(kpath, flags, mode, &file);
	if(result)
		return EFAULT;
	// place the file into curproc's file table
	result = filetable_place(curproc->p_filetable, file, retval);
	if(result)
		return ENFILE;
	kfree(kpath);
	return result;

	/*(void) upath; // suppress compilation warning until code gets written
	(void) flags; // suppress compilation warning until code gets written
	(void) mode; // suppress compilation warning until code gets written
	(void) retval; // suppress compilation warning until code gets written
	(void) allflags; // suppress compilation warning until code gets written
	(void) kpath; // suppress compilation warning until code gets written
	(void) file; // suppress compilation warning until code gets written
	*/
}

/*
 * read() - read data from a file
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
       int result = 0;
       struct iovec iov;
	// constuct a struct: uio
       struct uio useruio;
       struct openfile *file;

	// translate a fd number to an open file object
	result = filetable_get(curproc->p_filetable, fd, &file);
	if (result)
	   return result;

	// lock the seek position in the open file
	lock_acquire(file->of_offsetlock);

	// check for files opened write-only
	if(file->of_accmode == O_WRONLY)
	{
		lock_release(file->of_offsetlock);
		return EBADF;
	}
	uio_kinit(&iov, &useruio, buf, size, file->of_offset, UIO_READ);
	// call VOP_READ	
	result = VOP_READ(file->of_vnode, &useruio);
	if (result)
		return result;
	// update seek position
	file->of_offset = useruio.uio_offset;
	// unlock and filetable_put
	lock_release(file->of_offsetlock);	
	filetable_put(curproc->p_filetable, fd, file);
	// set return value correctly
	*retval = size - useruio.uio_resid;

       /*(void) fd; // suppress compilation warning until code gets written
       (void) buf; // suppress compilation warning until code gets written
       (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written
	*/
       return result;
}
/*
 * write() - write data to a file
 */
int
sys_write(int fd, userptr_t buf, size_t size, int *retval)
{
	struct iovec iov;
	struct uio useruio;
	struct openfile *file;
	int result = 0;
	int size1 = size;
	size_t size2;
	char *buff2 = (char*)kmalloc(size);

	  // translate a fd number to an open file object
        result = filetable_get(curproc->p_filetable, fd, &file);
        if (result)
           return result;

        // lock the seek position in the open file
        lock_acquire(file->of_offsetlock);

        // check for files opened read-only
        if(file->of_accmode == O_RDONLY)
        {
                lock_release(file->of_offsetlock);
                return EBADF;
        }
	copyinstr((userptr_t)buf, buff2, size1, &size2);
        uio_kinit(&iov, &useruio, buff2, size, file->of_offset, UIO_WRITE);
        // call VOP_WRITE
        result = VOP_WRITE(file->of_vnode, &useruio);
        if (result)
                return result;
        // update seek position
        file->of_offset = useruio.uio_offset;
        // unlock and filetable_put
        lock_release(file->of_offsetlock);
        filetable_put(curproc->p_filetable, fd, file);
        // set return value correctly
        *retval = size - useruio.uio_resid;

       /*(void) fd; // suppress compilation warning until code gets written
       (void) buf; // suppress compilation warning until code gets written
       (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written
        */
       return result;

}

/*
 * close() - remove from the file table.
 */
int 
sys_close(int fd)
{
	struct openfile *prevfile;
	struct openfile *file = NULL;
	// validate fd number (use filetable_okfd)
	if(filetable_okfd(curproc->p_filetable, fd))
	{
		filetable_placeat(curproc->p_filetable, file, fd, &prevfile);
	// use filetable_placeat to replace curproc's file table entry with NULL
	// check if previous entry in the file table was also NULL
	if (prevfile != NULL)
		openfile_decref(prevfile);
	}
	// decref the open file returned by filetable_placeat
	return 0;
}

/* 
* meld () - combine the content of two files word by word into a new file
*/
//int
//sys_meld(const_userptr_t path1, const_userptr_t path2, const_userptr_t path3)
//{

	/*
		int result = 0;

	// ** return result if != 0
	struct openfile *file1;
	struct openfile *file2;
	struct openfile *file3;
	int *retval = -1;
	// **  get path length for path1, path2, path3

	int len1 = strlen(path1);
	int len2 = strlen(path2);
	int len3 = strlen(path3);

	// ** allocate space to open files with kmalloc
	
	char *buf1 = (char*)kmalloc(len1);
	char *buf2 = (char*)kmalloc(len2);
	char *buf3 = (char*)kmalloc(len3);
	
	// copy in the supplied pathnames

	result = copyinstr((userptr_t)buf1, len1, &file1);
	if (result)
		return EBADF;
	result = copyinstr((userptr_t)buf2, len2, &file2);
	if (result)
		return EBADF;
	result = copyinstr((userptr_t)buf3, len3, &file3);
	if (result)
		return EBADF;

	// open the first two files (use openfile_open) for reading

	result = openfile_open(path1, O_EXCL|O_CREAT|O_APPEND|O_WRONLY,  222, &file1);

	// return if any file is not opened correctly

	if (result)
		return EBADF;
	result = openfile_open(path2, O_EXCL|O_CREAT|O_APPEND|O_WRONLY, 222, &file2); 
	if (result)
		return EBADF;

	// ** incref on first two openfiles

	openfile_incref(&file1);
	openfile_incref(&file2);

	// ** if first two files == O_WRONLY || openfile3 == O_RDONLY)
	// ** return EFTYPE
	if(file1->of_accmode == O_WRONLY || file1->of_accmode == O_RDONLY)
		return EFTYPE;
	if(file2->of_accmode == O_WRONLY || file2->of_accmode == O_RDONLY)
		return EFTYPE;

	// place them into curproc's file table (use filetable_place)
	
	result = filetable_place(curproc->p_filetable, file1, retval);
	if (result)
		return EBADF;
	result = filetable_place(curproc->p_filetable, file2, retval);
	if (result)
		return EBADF;

	// ** same as open

	// ** run a loop until retval of both files are 0

	// ** call sysread to read 4 bytes by passing in as param

	// ** call syswrite to write size of buffer using strlen(readbuffer)

	// refer to sys_read() for reading the first two files

	// refer to sys_write() for writing the third file

	// refer to sys_close to complete the use of three files

	// set the return value correctly for successful completion
	

	*/

	//void(path1);
	//void(path2);
	//void(path3);

	//return result;
//}
