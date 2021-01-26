#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>


#define DEV_MEM_SIZE 512

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

dev_t device_number;
struct cdev pcd_class;

/*buffer size*/
char device_buffer[DEV_MEM_SIZE];

/*cdev variable*/
struct cdev pcd_cdev;

struct class *class_pcd;

struct device *device_pcd;



loff_t pcd_lseek(struct file *filp, loff_t off,int whence)
{
pr_info("lseek requested\n");
return 0;
}


int pcd_open(struct inode* inode,struct file* filp)
{
pr_info("open requested\n");

return 0;
}




ssize_t pcd_read(struct file *filp, char __user* buff, size_t count, loff_t *f_pos)
{
 
pr_info("read_requested\n");
pr_info("Number of byte to be read= %zu bytes\n",count);
pr_info("initial file position = %lld\n", *f_pos);


/*check the count*/
if((count + *f_pos) > DEV_MEM_SIZE)
count= DEV_MEM_SIZE - *f_pos;


/*copy to user*/
if(copy_to_user(buff,&device_buffer[*f_pos],count))
return -EFAULT;


/*update current file position*/
*f_pos=count + *f_pos;


pr_info("Number of byte sucessfully read= %zu bytes\n",count);
pr_info("updated file position = %lld\n", *f_pos);


return count;
}





ssize_t pcd_write(struct file *filp,const char __user* buff, size_t count, loff_t *f_pos)
{

pr_info("write_requested\n");


/*check the count*/
if((count + *f_pos) > DEV_MEM_SIZE)
count=DEV_MEM_SIZE-*f_pos;

if(!count)
return -ENOMEM;

/*copy from user*/
if(copy_from_user(&device_buffer[*f_pos],buff,count))

return -EFAULT;

/*update current file position*/
*f_pos=count + *f_pos;


return count;
}


int pcd_release(struct inode* inode , struct file *filp)
{ 
pr_info("file_release_requested\n\n");

return 0;
}




/*pcd file operation*/ 
struct file_operations pcd_fops= {

		.open= pcd_open,
		.write = pcd_write,
		.read =  pcd_read,
		.llseek = pcd_lseek,
		.release = pcd_release,
		.owner  =  THIS_MODULE

};





static int __init pcd_driver_init(void)
{
int ret;
/*dynamic device creation*/
ret=alloc_chrdev_region(&device_number,0,1,"pcd_devices");
	if(ret<0)
	goto out;

/*cdev initialization with fops*/
cdev_init(&pcd_cdev,&pcd_fops);
	

/*cdev resistration with VFS*/
pcd_cdev.owner=THIS_MODULE;
ret=cdev_add(&pcd_cdev,device_number,1);
if(ret<0)
{
pr_err("char_dev resistration failed\n");
goto unregister_chardev;
}
	


/*udev class create*/
class_pcd=class_create(THIS_MODULE,"pcd_class");
if(IS_ERR(class_pcd))
{
pr_err("class creation failed\n");
ret=PTR_ERR(class_pcd);
goto cdev_del;
}


/*udev device create*/
device_pcd=device_create(class_pcd,NULL,device_number,NULL,"pcd");
 if(IS_ERR(device_pcd))
{
pr_err("Device creation failed\n");
ret=PTR_ERR(class_pcd);
goto device_destroy;
}

	pr_info("device initialation successfull\n");
	pr_info("device number : %d : %d \n", MAJOR(device_number), MINOR(device_number));


	return 0;
device_destroy:
	class_destroy(class_pcd);

cdev_del:
	cdev_del(&pcd_cdev);
	
unregister_chardev:
	unregister_chrdev_region(device_number,1);
	
out: 
	return ret;
}




static void __exit pcd_driver_exit(void)
{

device_destroy(class_pcd,device_number);
class_destroy(class_pcd);
cdev_del(&pcd_cdev);
unregister_chrdev_region(device_number,1);

pr_info("device_exit_function\n");
}




module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Pseudo_driver_code_implimention\n");
MODULE_INFO(board,"raspberry pi");
