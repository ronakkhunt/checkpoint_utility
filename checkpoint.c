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
#include <asm/ptrace.h>
int dumpHeader(struct file *f, struct task_struct *tsk, int *vmareas, int *openfiles, int *totalpages);

typedef struct Header_t
{
  int noofvmareas;
  int noofopenfiles;
  int noofpagessofar;
}Header_t;


int (*HandleMMFault)(struct mm_struct *mm,
					struct vm_area_struct * vma,
					unsigned long address,
					int write_access)=(void*)0xc0124d40;

int kwrite (struct file *f, char * buf, int size)
{
	int written;
	mm_segment_t oldmm;
	//Save current fs
 	oldmm = get_fs();
 	set_fs (get_ds());
	written=f->f_op->write(f,buf,size,&f->f_pos);
	//Restore fs
 	set_fs (oldmm);
 	return written;
}

void *GetTaskUserData(struct task_struct *tsk,struct vm_area_struct *vma,unsigned long virtual_address)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *page_number;


	unsigned long offset;
	void *physical_address;
	struct page *page;
	unsigned long framenum;

	printk(KERN_INFO "IN GetTAskUserData()1\n");
	pgd=pgd_offset(tsk->mm,virtual_address);

	printk(KERN_INFO "IN GetTAskUserData()2\n");
	pud = pud_offset(pgd, virtual_address);

	printk(KERN_INFO "IN GetTAskUserData()3\n");
	pmd=pmd_offset(pud,virtual_address);

	printk(KERN_INFO "IN GetTAskUserData()4\n");
	page_number=pte_offset_kernel(pmd,virtual_address);

	
	printk(KERN_INFO "IN GetTAskUserData()5\n");	

	if(page_number==NULL)
	{
		printk(KERN_INFO "IN GetTAskUserData()6\n");
		printk(KERN_INFO "\nGetTaskUserData():page not found\n");
		return NULL;
	}

	printk(KERN_INFO "IN GetTAskUserData()X\n");	

/*	if(pte_present(*page_number))
			printk(KERN_INFO "IN GetTAskUserData()X1\n");	
	else
			printk(KERN_INFO "IN GetTAskUserData()X2\n");	
	if(!pte_present(*page_number))
	{
		printk(KERN_INFO "IN GetTAskUserData()7\n");
		HandleMMFault(tsk->mm,vma,virtual_address,vma->vm_flags&VM_MAYWRITE);
		printk(KERN_INFO "IN GetTAskUserData()8\n");
	}
	printk(KERN_INFO "IN GetTAskUserData()Y\n");	

	if(!pte_present(*page_number))
	{
		printk(KERN_INFO "IN GetTAskUserData()9\n");
		printk("KERN_INFO  \nGetTaskUserData():page not found\n");		
		return NULL;
	}
*/
	
	page=pte_page(*page_number);
	printk(KERN_INFO "IN GetTAskUserData()10\n");
	framenum=(unsigned long)page_address(page);
	printk(KERN_INFO "IN GetTAskUserData()11\n");
	offset=(virtual_address)&(PAGE_SIZE-1);
	printk(KERN_INFO "IN GetTAskUserData()12\n");
	physical_address=(void*)(framenum+offset);
	printk(KERN_INFO "IN GetTAskUserData()13\n");
	return physical_address;
}

int StorePageList(struct file *f,struct task_struct *tsk,struct vm_area_struct *vma)
{
	void *tsk_user_data;
	unsigned long addr=0;
	for(addr=vma->vm_start;addr<vma->vm_end;addr+=PAGE_SIZE)
	{
		tsk_user_data=GetTaskUserData(tsk,vma,addr);
		if(tsk_user_data)
		{
			if((kwrite(f,tsk_user_data,PAGE_SIZE)!=PAGE_SIZE))
			{
				printk(KERN_INFO  "\nerror in page writing");
				return -1;
			}

		}
		else
		{
			printk(KERN_INFO  "\nStorePageList():Page not found...........\n");
		}

	}
	return 1;
}


int DumpVMAreas(struct file *f,struct task_struct *tsk,mm_segment_t fs)
{
	int i=0;
	struct vm_area_struct *vma;
	for(vma=tsk->mm->mmap;vma!=NULL;vma=vma->vm_next)
	{
		i++;
		kwrite(f,(void*)vma,sizeof(struct vm_area_struct));
		StorePageList(f,tsk,vma);
	}
	return 1;
}


void PrintMMStructure(struct mm_struct *mm)
{
  printk(KERN_INFO "Memory Map Structure\n");
  printk(KERN_INFO "mm->start_code =%ld\n",mm->start_code);
  printk(KERN_INFO "mm->end_code  =%ld\n",mm->end_code);
  printk(KERN_INFO "mm->start_data =%ld\n",mm->start_data);
  printk(KERN_INFO "mm->start_brk  =%ld\n",mm->start_brk);
  printk(KERN_INFO "mm->brk        =%ld\n",mm->brk);
  printk(KERN_INFO "mm->start_stack=%ld\n",mm->start_stack);
  printk(KERN_INFO "mm->arg_start  =%ld\n",mm->arg_start);
  printk(KERN_INFO "mm->arg_end    =%ld\n",mm->arg_end);
  printk(KERN_INFO "mm->env_start  =%ld\n",mm->env_start);
  printk(KERN_INFO "mm->env_end    =%ld\n",mm->env_end);
}

int dumpMMStructure(struct file *f,struct task_struct *tsk)
{
  struct mm_struct *pmm;
  int w;
  pmm=tsk->mm;
  w = kwrite(f,(void*)pmm,sizeof(struct mm_struct));
  printk("\nDumpMMStructure():Dumping mm_struct total %d\n",w);
  PrintMMStructure(pmm);
  return 1;
}

int dumpTaskStructure(struct file *f, struct task_struct *tsk)
{
	int written;
	//Write task_struct
	written = kwrite(f, (void *)tsk, sizeof(struct task_struct));
	return written;
}



int GetVMACount(struct task_struct *tsk)
{
  int i=0;
  struct vm_area_struct *vma;
  	printk(KERN_INFO "IN VMA COUNT");
  for(vma=tsk->mm->mmap;vma!=NULL;vma=vma->vm_next)
        i++;
  return i;
}

int GetPageCount(struct task_struct *tsk,unsigned long start,unsigned long end)
{
  int pagecount=0;
  int i,j,k,l;
  pgd_t *pgd;
  pud_t *pud;
  pmd_t *pmd;
  pte_t *pte;
  pgd=pgd_offset(tsk->mm,0);
	printk(KERN_INFO "IN PAGE COUNT\n");

	for(i=pgd_index(start);i<pgd_index(end);i++)
	{
		if((pgd_val(pgd[i]) & _PAGE_PRESENT))
		{
			pud = pud_offset(pgd+i, 0);
			for(l=0; l<PTRS_PER_PUD ;l++)
			{
				pmd=pmd_offset(pud+l,0);
				for(j=0;j<PTRS_PER_PMD;++j)
				{
					if(pmd_present(pmd[i]))
					{
						pte=pte_offset_kernel((pmd+j),0); //changed from pte_offset to pte_offset_kernel
						for(k=0;k<PTRS_PER_PTE;k++)
							if(pte_present(pte[i]))
								pagecount++;
					}
				}
			}
		}
	}
  
	  	printk(KERN_INFO "PAGE COUNT = %d\n",pagecount);
		return pagecount;
}

int GetOpenFileCount(struct files_struct *f)
{
	int c = atomic_read(&f->count);
	
		printk(KERN_INFO "IN OPEN FILE COUNT\n");
		printk(KERN_INFO "FILE COUNT %d\n", c);
  	return c;

}
int dumpHeader(struct file *f, struct task_struct *tsk, int *vmareas, int *openfiles, int *totalpages)
{
	  Header_t header;
	  printk(KERN_INFO "IN DUMP HEADER");
	  header.noofvmareas=*vmareas=GetVMACount(tsk);
	  header.noofopenfiles=*openfiles=GetOpenFileCount(tsk->files);
	  header.noofpagessofar=*totalpages=GetPageCount(tsk,0,PAGE_OFFSET);
	  printk(KERN_INFO "Wrriten header : %d",kwrite(f,(void*)&header,sizeof(Header_t)));
	  printk("\nDumpHeader():Dumping Header->noofvmareas=%d,noofopenfiles=%d\n",*vmareas,*openfiles);
//	  PrintHeader(ht);
	  return 1;
}

struct pt_regs GetRegisters(struct task_struct *tsk)
{
  struct pt_regs prset;
  	printk(KERN_INFO "In getReg\n");
  prset= *(
  			(
  				(struct pt_regs *) (THREAD_SIZE + (unsigned long)tsk)  				
  			) - 1
  		);
  return(prset);
}

int DumpRegisters(struct file *f,struct pt_regs prset)
{
  printk(KERN_INFO "In DumpReg\n");
  kwrite(f,(void*)&prset,sizeof(struct pt_regs));
  return 1;
}

int dumpAll(struct file *f, struct task_struct *tsk)
{
	mm_segment_t fs;
	int vmareas, openfiles, pagessofar;
	struct pt_regs prset;

	printk(KERN_INFO "\nDumpStructure():Dumping structure....................\n");
	prset=GetRegisters(tsk);
	fs=get_fs();
	dumpHeader(f,tsk,&vmareas,&openfiles,&pagessofar);
	dumpTaskStructure(f,tsk);
	dumpMMStructure(f,tsk);
	DumpVMAreas(f,tsk,fs);
	DumpRegisters(f,prset);
	kwrite(f,NULL,0);
	set_fs(fs);
	return 1;

}
int checkpoint(int pid)
{

/*************************Initialization of variables******************************/
	struct task_struct *tsk,*st;
	int checkpoint_flag = 0;
	char filename[100];
	char msg[256];
	struct file *f;

/****************************End of initialization*********************************/

	//Get the task_struct of 1st process
	st=tsk=&init_task;
	
   	printk(KERN_INFO "Begin\n");
	//file where the task_struct will be dumped change address according to file system

	snprintf(filename, 100, "/home/ahzaz/checkpoint.%d",pid);

	//Get task_struct of process with pid passed to function
    	do
    	{
		if(tsk->pid==pid)
            	{
			checkpoint_flag = 1;
			break;
		}
		tsk=next_task(tsk);
    	}while(tsk!=st);
	
	//if task_struct is found
	if(checkpoint_flag == 1)
	{
		send_sig(SIGSTOP,tsk,0);

		snprintf(msg, 1024, "Found, I am  %s with PID %d and my parent is %s with PID %d \n",tsk->comm, tsk->pid, (tsk->parent)->comm, 																(tsk->parent)->pid);
	   	printk(KERN_INFO "%s", msg);
		//open file to for dumping		
		f=filp_open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777);
		if(!f)
		{
			printk("file error\n");
			return -1;
		}
		else
		{
			//Dump All
			if(!dumpAll(f, tsk)) {
				printk("task struct error write error\n");
				checkpoint_flag = 0;
			}

		}
  		send_sig(SIGKILL,tsk,0);


	}
	//Some error Occured
	if(checkpoint_flag == 0)
		printk(KERN_INFO "Error\n");

	return checkpoint_flag;
}

//This is called from userspace when user calls write on checkpoint file in /proc
int MyInput(struct file  *fp, char *buff, unsigned long len,loff_t *off)
{
	pid_t pid;
	printk(KERN_INFO "In MyInput");
	get_user(pid,(unsigned long*)buff);
	return checkpoint(pid); 
}

static struct proc_dir_entry *pentry;

//Called When Module Is Inserted
static int __init init_mod(void)
{
	//Create a proc entry named checkpoint
	//Write call to this file will invoke MyInput() function.
	pentry=create_proc_entry("checkpoint", S_IFREG|S_IWUGO, NULL);
	pentry->write_proc = MyInput;
	printk(KERN_INFO "Module Inserted");
	return 0; 
}

//Called When Module Is Removed
static void __exit hello_exit(void)
{
	//Remove checkpoint entry from proc
	remove_proc_entry("checkpoint",NULL);
	printk(KERN_INFO "Bye \n");
}


module_init(init_mod);
module_exit(hello_exit);
MODULE_LICENSE("GPL");

