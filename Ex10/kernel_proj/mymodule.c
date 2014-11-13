#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/proc_fs.h>

int procfile_read(char *buffer, 
		char **buffer_location, 
		off_t offset,
		int buffer_length, 
		int *eof, 
		void *data);

static struct proc_dir_entry* myproc;

static int __init mymodule_init(void)
{
	printk(KERN_INFO "Module initing 2\n");
	myproc = create_proc_entry("hack", 0644, NULL);
	myproc->read_proc = procfile_read;


	return 0;
}

static void __exit mymodule_exit(void)
{
	printk(KERN_INFO "Goodbye world 2.\n");
	remove_proc_entry("hack", NULL);
}

int procfile_read(char *buffer, 
		char **buffer_location, 
		off_t offset,
		int buffer_length, 
		int *eof, 
		void *data){
	if (offset > 0){
		return 0;
	}else{
		return sprintf(buffer, "You cracked me! You 1337 haxxor!\n");
	}
}
MODULE_LICENSE("GPL");
module_init(mymodule_init);
module_exit(mymodule_exit);

