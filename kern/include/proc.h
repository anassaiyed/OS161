#ifndef _PROC_H_
#define _PROC_H_

struct proc_table{

  pid_t pid;
  pid_t ppid;
  struct thread *proc_thread;
  bool status;
  int exit_code;
  struct lock *proc_lock;
  struct cv *proc_cv;
 };


// function listings

// sys_fork
// sys_execv
// sys_waitpid

pid_t makepid(struct thread *pid_thread);
int justpid(int *retval);
int sys_fork(struct trapframe *tf,int *retval);
int waitpid(pid_t pid, int *status, int options,int *retval);
void sys_exit(int exitcode);
void mywait(struct proc_table *pro);



#endif /* proc_h_ */

