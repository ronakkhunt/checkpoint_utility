#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/binfmts.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/mman.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <linux/sys.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <asm/pgtable.h>
#include <linux/fdtable.h>
#include <asm/syscalls.h>
#include <asm/ptrace.h>
#include <linux/cred.h>
#include <linux/mm.h>



static struct proc_dir_entry *pentry;


typedef struct Header_t
{
  int noofvmareas;
  int noofopenfiles;
  int noofpagessofar;
}Header_t;


int restoreHeader(struct file *f, int *vmareas, int *openfiles, int *totalpages)
{
	Header_t header;
	printk(KERN_INFO "IN restore HEADER");	  
	printk(KERN_INFO "Wrriten header : %d",f->f_op->read(f, (char *)&header,sizeof(Header_t), &f->f_pos));	  
	  
	*vmareas =  header.noofvmareas;
	*openfiles=header.noofopenfiles;
	*totalpages=header.noofpagessofar;
	  
//	  PrintHeader(ht);*/
	  return 1;
}

int restoreTaskStructure(struct file *f)
{
	int read=1;
	struct task_struct *ptsk;
	struct cred *creds;
	//Write task_struct
	f->f_op->read(f, (char *)ptsk, sizeof(struct task_struct), &f->f_pos);
	  //pcurrent->uid=current->uid;
	creds = current_cred();
	  creds->gid=ptsk->cred->gid;
	  creds->euid=ptsk->cred->euid;
	  creds->egid=ptsk->cred->egid;
	  creds->suid=ptsk->cred->suid;
	  creds->sgid=ptsk->cred->sgid;
	  creds->fsuid=ptsk->cred->fsuid;
	  creds->fsgid=ptsk->cred->fsgid;
	  strcpy(current->comm,ptsk->comm);
	return read;
}

int restoreMMStructure(struct file *f)
{
  struct mm_struct *pmm,mm;
  printk("\nReadMMStructure():restoring memory map structure\n");
  f->f_op->read(f,(void*)&mm,sizeof(mm),&f->f_pos);
  pmm=current->mm;
  pmm->start_code =mm.start_code;
  pmm->end_code  =mm.end_code;
  pmm->start_data =mm.start_data;
  pmm->end_data  =mm.end_data;
  pmm->start_brk  =mm.start_brk;
  pmm->brk        =mm.brk;
  pmm->start_stack=mm.start_stack;
  pmm->arg_start  =mm.arg_start;
  pmm->arg_end    =mm.arg_end;
  pmm->env_start  =mm.env_start;
  pmm->env_end    =mm.env_end;
  return 1;
	
}
int restoreVMAreas(struct file *f,mm_segment_t fs,int noOfVMAreas)
{
  struct vm_area_struct vma;
  int i=0;
  unsigned long sizeofvma=0;
  unsigned long vmaprot=0,vmaflags=0;
  printk("\nReadVMAReas():Restoring Virtual Memory Areas.................\n");

  for(i=0;i<noOfVMAreas;i++)
  {
    set_fs(KERNEL_DS);
    f->f_op->read(f,(void*)&vma,sizeof(vma),&f->f_pos);
    printk("\nf->f_pos=%d",f->f_pos);
    sizeofvma=vma.vm_end-vma.vm_start;
    set_fs(fs);
    vmaprot=0;
    if(vma.vm_flags&VM_READ)
          vmaprot|=PROT_READ;
    if(vma.vm_flags&VM_WRITE)
          vmaprot|=PROT_WRITE;
    if(vma.vm_flags&VM_EXEC)
          vmaprot|=PROT_EXEC;
    vmaflags=MAP_FIXED|MAP_PRIVATE;
    if(vma.vm_flags&VM_GROWSDOWN)
          vmaflags|=MAP_GROWSDOWN;
    if(vma.vm_flags&VM_DENYWRITE)
          vmaflags|=MAP_DENYWRITE;
//    if(vma.vm_flags&VM_EXECUTABLE)
//          vmaflags|=MAP_EXECUTABLE;

    if((vm_mmap(f,vma.vm_start,sizeofvma,vmaprot,vmaflags,f->f_pos))!=vma.vm_start)
    {
        printk("\nrestoreVMAreas():do_mmap error \n");
    }
    f->f_pos+=sizeofvma;
  }
  set_fs(KERNEL_DS);
  return 1;
}


int restoreRegisters(struct file *f,struct pt_regs *prset)
{
	printk("\nReadRegisters():Restoring registers.............\n");
	f->f_op->read(f, (void*)prset, sizeof(*prset), &f->f_pos);
	*(((struct pt_regs *) (THREAD_SIZE + (unsigned long)current)) - 1)=*prset;
	return 0;
}

int restoreAll(struct file *f)
{
	mm_segment_t fs;
	int vmareas, openfiles, pagessofar;
	struct pt_regs prset;

	printk(KERN_INFO "\nrestore Structure():restoring structure....................\n");
	fs=get_fs();
	set_fs(KERNEL_DS);
	restoreHeader(f,&vmareas,&openfiles,&pagessofar);
	restoreTaskStructure(f);
	restoreMMStructure(f);
	restoreVMAreas(f,fs,vmareas);
	restoreRegisters(f,&prset);
	set_fs(fs);
	return 1;

}

int restore(char *filename)
{
	struct task_struct *current;
	int restore_flag = 0;
	char msg[256];
	struct file *f;
//	struct pt_regs prset;
	mm_segment_t fs;
//	pid_t pid = current->pid;

/****************************End of initialization*********************************/

	printk(KERN_INFO "Begin\n");
	
	//if task_struct is found
	send_sig(SIGSTOP,current,0);
	snprintf(msg, 1024, "Restore : Found, I am  %s with PID %d and my parent is %s with PID %d \n",current->comm, current->pid, (current->parent)->comm, 																(current->parent)->pid);
   	printk(KERN_INFO "%s", msg);
	//open file to for restoreing		
	f=filp_open(filename,O_RDONLY,0777);
	if(!f)
	{
		printk("file error\n");
		return -1;
	}
	else
	{
		fs=get_fs();
		set_fs(KERNEL_DS);
			if(!restoreAll(f)) {
			printk("task struct error write error\n");
			restore_flag = 0;
		}
		set_fs(fs);
	}
	send_sig(SIGCONT,current,0);
	//Some error Occured
	if(restore_flag == 0)
		printk(KERN_INFO "Error\n");

	return restore_flag;

}
int MyInput(struct file  *fp, char *buff, unsigned long len,loff_t *off)
{
	pid_t pid;
	printk(KERN_INFO "In MyInput\n");
	pid = restore(buff); 
	if(pid != -1)
		printk(KERN_INFO "CAN NOT Restore\n");
	else
		printk(KERN_INFO "Restored\n");
	return pid;
}

static int __init init_mod(void)
{
	//Create a proc entry named checkpoint
	//Write call to this file will invoke MyInput() function.
	pentry=create_proc_entry("restore", S_IFREG|S_IWUGO, NULL);
	pentry->write_proc = MyInput;
	printk(KERN_INFO "Module Inserted");
	return 0; 
}

//Called When Module Is Removed
static void __exit hello_exit(void)
{
	//Remove checkpoint entry from proc
	remove_proc_entry("restore",NULL);
	printk(KERN_INFO "Bye \n");
}


module_init(init_mod);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
