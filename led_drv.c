#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/io.h>

int major; //定义主设备号的变量
int kdata = 0;

#define GPF3CON (phys_addr_t)0x114001e0 // led4  5
#define GPF3DAT (phys_addr_t)0x114001e4
#define GPX1CON (phys_addr_t)0x11000c20 // led3
#define GPX1DAT (phys_addr_t)0x11000c24
#define GPX2CON (phys_addr_t)0x11000c40 // led2
#define GPX2DAT (phys_addr_t)0x11000c44

unsigned int *led_4_5_ctrl_base = NULL; //虚拟的led2控制寄存器
unsigned int *led_4_5_data_base = NULL; //虚拟的led2数据寄存器
unsigned int *led3_ctrl_base = NULL; //虚拟的led2控制寄存器
unsigned int *led3_data_base = NULL; //虚拟的led2数据寄存器
unsigned int *led2_ctrl_base = NULL; //虚拟的led2控制寄存器
unsigned int *led2_data_base = NULL; //虚拟的led2数据寄存器

void rgb_led_off(void)
{
	(*led_4_5_data_base) &= (~(1 << 5));
	(*led_4_5_data_base) &= (~(1 << 4));
	(*led3_data_base) &= (~(1));
	(*led2_data_base) &= (~(1 << 7));
}

int led_open(struct inode *inode, struct file *file)
{
	return 0;	
}

ssize_t led_read(struct file *file, char __user * ubuf, 
		size_t size, loff_t *offs)
{
	int ret;
	//如果用户想拷贝的字节的个数大于内核空间字节的个数
	//在内核空间直接纠正，即将内核空间的大小赋值给用户
	//空间
	if(size > sizeof(kdata)) size = sizeof(kdata);
	//将内核空间的数据拷贝到用户空间，参数1：用户空间的首地址
	//参数2：内核空间的首地址，参数3：拷贝数据的大小
	ret = copy_to_user(ubuf,&kdata,size);
	if(ret){
		printk("copy data to user error\n");
		//失败返回input/output 的错误码
		return -EIO;
	}
	
	//成功返回拷贝的字节的个数
	return size;	
}

ssize_t led_write(struct file *file, const char __user *ubuf,
		size_t size, loff_t *offs)
{
	int ret;
	//如果用户想写的字节的个数大于内核空间字节的个数
	//在内核空间直接纠正，即将内核空间的大小赋值给用户
	//想写的大小
	if(size > sizeof(kdata)) size = sizeof(kdata);
	//将用户空间的数据拷贝到内核空间，参数1：内核空间的首地址
	//参数2：用户空间的首地址，参数3：拷贝数据的大小
	ret = copy_from_user(&kdata,ubuf,size);
	if(ret){
		printk("copy data to user error\n");
		//失败返回input/output 的错误码
		return -EIO;
	}

	//根据kdata来决定亮哪一个灯，
	rgb_led_off();
	switch(kdata){

		case 2:
			(*led2_data_base) |= (1 << 7);
			break;
		case 3:
			(*led3_data_base) |= 1;
			break;
		case 4:
			(*led_4_5_data_base) |= (1 << 4);
			break;
		case 5:
			(*led_4_5_data_base) |= (1 << 5);
			break;
		default:
			printk("led all off\n");
			break;
	}
	//成功返回拷贝的字节的个数
	return size;	

}
int led_close(struct inode *inode, struct file *file)
{
	return 0;	
}

struct file_operations fops = {
	.open    = led_open,
	.read    = led_read,
	.write   = led_write,
	.release = led_close,
};
//入口函数
static int  led_init(void)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	//1.注册字符设备驱动
	major = register_chrdev(0,"hello",&fops);
	if(major < 0){
		printk("register char device error\n");
		return major;
	}

	printk("alloc major = %d\n",major);

	//2.映射要操作灯的地址，初始化灯
	//也可以直接把连续的需要用到的成片的物理地址映射出来

	led_4_5_ctrl_base = ioremap(GPF3CON,4);
	if(led_4_5_ctrl_base == NULL){
		printk("ioremap red led error\n");
		return -ENOMEM;
	}

	led_4_5_data_base = ioremap(GPF3DAT,4);
	if(led_4_5_data_base == NULL) {
		printk("ioremap red led error\n");
		return -ENOMEM;
	}

	led3_ctrl_base = ioremap(GPX1CON,4);
	if(led3_ctrl_base == NULL){
		printk("ioremap red led error\n");
		return -ENOMEM;
	}

	led3_data_base = ioremap(GPX1DAT,4);
	if(led3_data_base == NULL) {
		printk("ioremap red led error\n");
		return -ENOMEM;
	}

	led2_ctrl_base = ioremap(GPX2CON,4);
	if(led2_ctrl_base == NULL){
		printk("ioremap red led error\n");
		return -ENOMEM;
	}

	led2_data_base = ioremap(GPX2DAT,4);
	if(led2_data_base == NULL) {
		printk("ioremap red led error\n");
		return -ENOMEM;
	}

	//3.初始化led，并灭led
	
	*led_4_5_ctrl_base = (*led_4_5_ctrl_base & (~(0xFF << 16)) | (0x11 << 16));
	*led_4_5_data_base &= (~(1 << 5));
	*led_4_5_data_base &= (~(1 << 4));
	*led3_ctrl_base = (*led3_ctrl_base & (~(0xF)) | 1);
	*led3_data_base &= (~(1));
	*led2_ctrl_base = (*led2_ctrl_base & (~(0xF << 28))) | (0x1 << 28);
	*led2_data_base &= (~(1 << 7));

	return 0;
}

//出口函数
static void led_exit(void)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	//1.地址的注销
	
	iounmap(led_4_5_ctrl_base);
	iounmap(led_4_5_data_base);
	iounmap(led3_ctrl_base);
	iounmap(led3_data_base);
	iounmap(led2_ctrl_base);
	iounmap(led2_data_base);

	//2.注销字符设备驱动
	unregister_chrdev(major,"hello");
}
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL"); //模块的许可证

