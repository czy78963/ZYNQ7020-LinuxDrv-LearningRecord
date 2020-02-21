#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/ide.h>  
#include <linux/types.h>  
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/device.h>
#include <asm/uaccess.h>
  
/* 设备节点名称 */  
#define DEVICE_NAME       "gpio_leds"
/* 设备号个数 */  
#define DEVID_COUNT       1
/* 驱动个数 */  
#define DRIVE_COUNT       1
/* 主设备号 */
#define MAJOR
/* 次设备号 */
#define MINOR             0
  
/* gpio寄存器虚拟地址 */  
static u32 *GPIO_DIRM_0; 
/* gpio使能寄存器 */   
static u32 *GPIO_OEN_0;
/* gpio控制寄存器 */  
static u32 *GPIO_DATA_0;
/* AMBA外设时钟使能寄存器 */  
static u32 *APER_CLK_CTRL;
  
/* 把驱动代码中会用到的数据打包进设备结构体 */
struct alinx_char_dev{
	dev_t              devid;      //设备号
	struct cdev        cdev;       //字符设备
	struct class       *class;     //类
	struct device      *device;    //设备
	struct device_node *nd;        //设备树的设备节点
};
/* 声明设备结构体 */
static struct alinx_char_dev alinx_char = {
	.cdev = {
		.owner = THIS_MODULE,
	},
};

/* open函数实现, 对应到Linux系统调用函数的open函数 */  
static int gpio_leds_open(struct inode *inode_p, struct file *file_p)  
{  
	/* MIO_0时钟使能 */  
    *APER_CLK_CTRL |= 0x00400000;  
    /* MIO_0设置成输出 */  
    *GPIO_DIRM_0 |= 0x00000001;  
    /* MIO_0使能 */  
    *GPIO_OEN_0 |= 0x00000001;  
      
    printk("gpio_test module open\n");  
      
    return 0;  
}  
  
  
/* write函数实现, 对应到Linux系统调用函数的write函数 */  
static ssize_t gpio_leds_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)  
{  
    int rst;  
    char writeBuf[5] = {0};  
      
    printk("gpio_test module write\n");  
  
    rst = copy_from_user(writeBuf, buf, len);  
    if(0 != rst)  
    {  
        return -1;    
    }  
      
    if(1 != len)  
    {  
        printk("gpio_test len err\n");  
        return -2;  
    }  
    if(1 == writeBuf[0])  
    {  
        *GPIO_DATA_0 &= 0xFFFFFFFE;  
        printk("gpio_test ON\n");  
    }  
    else if(0 == writeBuf[0])  
    {  
        *GPIO_DATA_0 |= 0x00000001;  
        printk("gpio_test OFF\n");  
    }  
    else  
    {  
        printk("gpio_test para err\n");  
        return -3;  
    }  
      
    return 0;  
}  
  
/* release函数实现, 对应到Linux系统调用函数的close函数 */  
static int gpio_leds_release(struct inode *inode_p, struct file *file_p)  
{  
	printk("gpio_test module release\n");  
    return 0;  
}  
	  
/* file_operations结构体声明, 是上面open、write实现函数与系统调用函数对应的关键 */  
static struct file_operations ax_char_fops = {  
    .owner   = THIS_MODULE,  
    .open    = gpio_leds_open,  
    .write   = gpio_leds_write,     
    .release = gpio_leds_release,   
};  
  
/* 模块加载时会调用的函数 */  
static int __init gpio_led_init(void)  
{
	/* 用于接受返回值 */
	u32 ret = 0;
	/* 存放reg数据的数组 */
	u32 reg_data[10];
	
	/* 通过节点名称获取节点 */
	alinx_char.nd = of_find_node_by_name(NULL, "alinxled");
	/* 4、获取 reg 属性内容 */
	ret = of_property_read_u32_array(alinx_char.nd, "reg", reg_data, 8);
	if(ret < 0) 
	{
		printk("get reg failed!\r\n");
		return -1;
	} 
	else 
	{
		/* do nothing */
	}
	
	/* 把需要修改的物理地址映射到虚拟地址 */
	GPIO_DIRM_0   = ioremap(reg_data[0], reg_data[1]);  
	GPIO_OEN_0    = ioremap(reg_data[2], reg_data[3]);
	GPIO_DATA_0   = ioremap(reg_data[4], reg_data[5]);
	APER_CLK_CTRL = ioremap(reg_data[6], reg_data[7]);
	
	/* 注册设备号 */
	alloc_chrdev_region(&alinx_char.devid, MINOR, DEVID_COUNT, DEVICE_NAME);
	
	/* 初始化字符设备结构体 */
	cdev_init(&alinx_char.cdev, &ax_char_fops);
	
	/* 注册字符设备 */
	cdev_add(&alinx_char.cdev, alinx_char.devid, DRIVE_COUNT);
	
	/* 创建类 */
	alinx_char.class = class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(alinx_char.class)) 
	{
		return PTR_ERR(alinx_char.class);
	}
	
	/* 创建设备节点 */
	alinx_char.device = device_create(alinx_char.class, NULL, 
                                      alinx_char.devid, NULL, 
					                  DEVICE_NAME);
	if (IS_ERR(alinx_char.device)) 
	{
		return PTR_ERR(alinx_char.device);
	}
	
    return 0;  
}

/* 卸载模块 */  
static void __exit gpio_led_exit(void)  
{  
    /* 注销字符设备 */
    cdev_del(&alinx_char.cdev);
    
    /* 注销设备号 */
    unregister_chrdev_region(alinx_char.devid, DEVID_COUNT);
    
    /* 删除设备节点 */
    device_destroy(alinx_char.class, alinx_char.devid);
    
    /* 删除类 */
    class_destroy(alinx_char.class);
   
    /* 释放对虚拟地址的占用 */  
    iounmap(GPIO_DIRM_0);
    iounmap(GPIO_OEN_0);
    iounmap(GPIO_DATA_0);
    iounmap(APER_CLK_CTRL);
       
    printk("gpio_led_dev_exit_ok\n");  
}  
  
/* 标记加载、卸载函数 */  
module_init(gpio_led_init);  
module_exit(gpio_led_exit);  
  
/* 驱动描述信息 */  
MODULE_AUTHOR("Alinx");  
MODULE_ALIAS("gpio_led");  
MODULE_DESCRIPTION("DEVICE TREE GPIO LED driver");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  

