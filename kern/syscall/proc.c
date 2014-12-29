#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <kern/wait.h>
#include <proc.h>
#include <file.h>
#include <synch.h>
#include <spl.h>

struct proc_table* proc[20];

 
pid_t makepid(struct thread *pid_thread)
{
  pid_t i;
  pid_t pid;

  int max_pid = 20;
  
  KASSERT(proc[0]==NULL); 
  
  for(i=2;i<=20;i++)
   {
    if(proc[i]==NULL)
      {
        break;
      }
    
   }
   
  if(i==max_pid)
    {
      kprintf("\n At the limit");
      return ENPROC;
   }
   
  pid = i;
  proc[i] = (struct proc_table *)kmalloc(sizeof(struct proc_table));
   
  pid_thread->pid = pid;
  
 proc[i]->pid = pid;
 proc[i]->ppid = pid_thread->ppid;
 proc[i]->exit_code = 0;
 proc[i]->status = false;
 proc[i]->proc_lock = lock_create("Process_lock");
 proc[i]->proc_cv = cv_create("Process_cv");
 proc[i]->proc_thread = pid_thread;
 
 return pid;
 
}


int justpid(int *retval)
 {
   *retval =  curthread->pid;
   return 0;
 }



int sys_fork(struct trapframe *tf,int *retval)
{
  struct trapframe *temp_tf;
  struct addrspace *temp_addr;
  struct thread *child_process; 
   
  temp_tf = kmalloc(sizeof(struct trapframe *));
 
  bzero(temp_tf,sizeof(struct trapframe *));
  memcpy(temp_tf,tf,sizeof(struct trapframe *));
   
  temp_addr = kmalloc(sizeof(struct addrspace));
  
  int result;
  result = as_copy(curthread->t_addrspace,&temp_addr);
  if(result)
   {
     *retval = result;
      return ENOMEM;
   }

   int a = splhigh();
//   int thread_fork(const char *name,
  //              void (*func)(void *, unsigned long),
    //            void *data1, unsigned long data2,
      //          struct thread **ret);

    result = thread_fork("child_process",enter_forked_process,(struct trapframe *)temp_tf,(unsigned long)temp_addr,&child_process);

    if(result)
     {
       splx(a);
       return result;
     }

    int i = 3;

    for(i=3;i<OPEN_MAX;i++)
     {
        if(curthread->fd_table[i]!=NULL)
         {
             child_process->fd_table[i] = curthread->fd_table[i];
         }
         else
          break;
    }  

   
    child_process->ppid = curthread->pid;

    // function to get a pid for the child
   child_process->pid =  makepid(child_process);

    *retval = child_process->pid;

    splx(a);
    kfree(temp_tf);
    kfree(temp_addr);
    return 0;
     
}



int waitpid(pid_t pid,int *status,int options,int *retval)
{
  
  struct proc_table *singlep;
 // check if waiting on self
  if(pid == curthread->pid)
   {
    // *retval = -1; 
     return ECHILD;
   }
  
   singlep = proc[pid];
   // check if it is null 
   if(singlep  == NULL)
    {
    // *retval = -1;
     return ESRCH;
     }
   
   // need to check if given pid is that of the parent of the current thread
   if(singlep->pid == curthread->ppid)
     {
       // *retval = -1;
        return ECHILD;
       
     } 
  // need to check if any siblings are waiting 
    // also verify that the parent is waiting on the child
   if(singlep->ppid != curthread->pid)
    {
      return ECHILD;
     // *retval = -1;
    }

   if(options!=0)
    {
       return EINVAL;
      // *retval = -1; 
    }   

   // need to wait for child to exit
 // put it into a function
 lock_acquire(singlep->proc_lock);
 
  while(singlep->status==0)
    {
     cv_wait(singlep->proc_cv,singlep->proc_lock);
    }
  
  *status = singlep->exit_code;

  proc[pid] = NULL;

  lock_release(singlep->proc_lock);
  
  thread_exit(); 
  
  *retval = pid;

  return 0;
}

void sys_exit(int exitcode)
{
  pid_t cur_pid;
 
  cur_pid = curthread->pid;
  
  lock_acquire(proc[cur_pid]->proc_lock);    
  
  
  proc[cur_pid]->status = 1;
  proc[cur_pid]->exit_code = _MKWAIT_EXIT(exitcode); 
  cv_broadcast(proc[cur_pid]->proc_cv,proc[cur_pid]->proc_lock);
 
  lock_release(proc[cur_pid]->proc_lock); 
   
}
 
void
mywait(struct proc_table *pro)
{
  while(pro->status == 0)
     {
          cv_wait(pro->proc_cv,pro->proc_lock);
     }
}


