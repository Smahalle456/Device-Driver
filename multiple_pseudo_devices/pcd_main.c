#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>


#define PCD_DEVICE_1 1024
#define PCD_DEVICE_2 512
#define PCD_DEVICE_3 1024
#define PCD_DEVICE_4 512

#define NO_OF_DEVICES 4

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

#define RDWR 0x11
#define RDONLY 0x01
#define RWONLY 0x10



/*buffer size*/
char device_buffer_pcdev1[PCD_DEVICE_1];
char device_buffer_pcdev2[PCD_DEVICE_2];
char device_buffer_pcdev3[PCD_DEVICE_3];
char device_buffer_pcdev4[PCD_DEVICE_4];

struct pcdev_private_data
{
char *buffer;
unsigned size;
const char *serial_no;
int perm;
struct cdev cdev; /*this member can be use to get the container address of member element*/

};


struct pcdrv_private_data
{
 int total_devices;
 dev_t device_number;
 struct class *class_pcd;
 struct device *device_pcd;
 struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};


struct pcdrv_private_data pcdrv_data={

	.total_devices=NO_OF_DEVICES,
	.pcdev_data={

		[0]={

			.buffer=device_buffer_pcdev1,
			.size=PCD_DEVICE_1,
			.serial_no = "pcd_device_1",
			.perm = RDONLY /*read_only*/
		

		},
		[1]={

			.buffer=device_buffer_pcdev2,
			.size=PCD_DEVICE_2,
		        .serial_no = "pcd_device_2",
			.perm = 0x10 /*write_only*/
		

		},
		[2]={

			.buffer=device_buffer_pcdev3,
			.size=PCD_DEVICE_3,
			.serial_no = "pcd_device_3",
			.perm = RDWR, /*read_write*/
		

		},
		[3]={

			.buffer=device_buffer_pcdev4,
			.size=PCD_DEVICE_4,
			.serial_no = "pcd_device_4",
			.perm = RDWR, /*read_write*/
		

		},	
	}
};


struct cdev pcd_class;


/*cdev variable*/
struct cdev pcd_cdev;





loff_t pcd_lseek(struct file *filp, loff_t off,int whence)
{
pr_info("lseek requested\n");
return 0;
}




ssize_t pcd_read(struct file *filp, char __user* buff, size_t count, loff_t *f_pos)
{

struct pcdev_private_data *pcdev_data;

int max_size;

pr_info("read is requested\n");
pr_info("Number of byte to be read= %zu bytes\n",count);
pr_info("initial file position = %lld\n", *f_pos);

pcdev_data=(struct pcdev_private_data*)filp->private_data;
max_size=pcdev_data->size;


/*check the count*/
if((count + *f_pos) > max_size)
count= max_size - *f_pos;


/*copy to user*/
if(copy_to_user(buff,pcdev_data->buffer+(*f_pos),count))
return -EFAULT;


/*update current file position*/
*f_pos=count + *f_pos;


pr_info("Number of byte sucessfully read= %zu bytes\n",count);
pr_info("updated file position = %lld\n", *f_pos);


return count;
 
}





ssize_t pcd_write(struct file *filp,const char __user* buff, size_t count, loff_t *f_pos)
{

struct pcdev_private_data *pcdev_data;
int max_size;
pr_info("write_requested\n");

pcdev_data=(struct pcdev_private_data*)filp->private_data;
max_size=pcdev_data->size;


/*check the count*/
if((count + *f_pos) > max_size)
count=max_size-*f_pos;

if(!count)
return -ENOMEM;

/*copy from user*/
if(copy_from_user(pcdev_data->buffer+(*f_pos),buff,count))

return -EFAULT;

/*update current file position*/
*f_pos=count + *f_pos;


return count;

}

int check_permission(int dev_perm, int access_mode)
{

if(dev_perm==RDWR)
	return 0;


else if((dev_perm==RDONLY) && ((access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE)))
	return 0;


else if((dev_perm==RWONLY) &&((access_mode & FMODE_WRITE) && !(access_mode & FMODE_READ)))
	return 0;

return -EPERM;

}



int pcd_open(struct inode* inode,struct file* filp)
{
int rret;
int minor_n;
struct pcdev_private_data *pcdev_data;

/*how to indentity which mior device has opened a request*/
minor_n= MINOR(inode->i_rdev);
pr_info("Device Minor Numbet= %d\n",minor_n);

/*private data structure of the device(to get the buffer and all kind of inforation of private)*/
/*container_of is the macro used to get the address of parent structure address arguments(ptr,type,member)*/

pcdev_data=container_of(inode->i_cdev,struct pcdev_private_data,cdev); 

/*file pointe's private data menber to be assed by other methods(read/write) */

filp->private_data=pcdev_data;


/* check permission*/

rret=check_permission(pcdev_data->perm,filp->f_mode);

(!rret)?pr_info("open is sucessful\n"):pr_info("open is unsuccessful\n");

pr_info("open is requested\n");


return rret;
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

int ret,i;
/*dynamic device creation*/
ret=alloc_chrdev_region(&pcdrv_data.device_number,0,NO_OF_DEVICES,"pcd_devices");
	if(ret<0){
	pr_err("alloc_chrdev_failed\n");
	goto out;
}



/*udev class create under /sys/class*/
pcdrv_data.class_pcd=class_create(THIS_MODULE,"pcd_class");
if(IS_ERR(pcdrv_data.class_pcd))
{
pr_err("class creation failed\n");
ret=PTR_ERR(pcdrv_data.class_pcd);
goto cdev_del;
}

for(i=0;i<NO_OF_DEVICES;i++){
pr_info("device MAJOR and MINOR number : %d : %d \n", MAJOR(pcdrv_data.device_number+i), MINOR(pcdrv_data.device_number+i));


/*cdev initialization with fops*/
cdev_init(&pcdrv_data.pcdev_data[i].cdev,&pcd_fops);
	

/*cdev resistration with VFS*/
pcdrv_data.pcdev_data[i].cdev.owner=THIS_MODULE;
ret=cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number,1);
if(ret<0)
{
pr_err("char_dev resistration failed\n");
goto unreg_chardev;

}
	

  
/*udev device create*/
pcdrv_data.device_pcd=device_create(pcdrv_data.class_pcd,NULL,pcdrv_data.device_number+i,NULL,"pcdev - %d",i+1);
if(IS_ERR(pcdrv_data.device_pcd))
{
pr_err("Device create failed\n");
ret=PTR_ERR(pcdrv_data.class_pcd);
goto class_del;
}

	pr_info("device initialation successfull\n");
	
}

	return 0;
cdev_del:
class_del:
for(;i>=0;i--){
 		device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
		}
		class_destroy(pcdrv_data.class_pcd);
unreg_chardev:
	unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
	
out: 
	pr_info("Module_insertion_failed\n");
	return ret;
}




static void __exit pcd_driver_exit(void)
{
int i;


for(i=0;i<NO_OF_DEVICES;i++){
device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
}
class_destroy(pcdrv_data.class_pcd);


unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);

pr_info("device_exit_function\n");

}




module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Pseudo_Multiple_device_implimentation\n");
MODULE_INFO(board,"raspberry pi");
