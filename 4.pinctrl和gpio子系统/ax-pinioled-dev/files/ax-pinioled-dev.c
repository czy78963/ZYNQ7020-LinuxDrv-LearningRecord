#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/ide.h>  
#include <linux/types.h>  
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/mach/map.h>
#include <asm/io.h>
  
/* 设备节点名称 */  
#define DEVICE_NAME       "gpio_leds"
/* 设备号个数 */  
#define DEVID_COUNT       1
/* 驱动个数 */  
#define DRIVE_COUNT       1
/* 主设备号 */
#define MAJOR1
/* 次设备号 */
#define MINOR1            0
/* LED点亮时输入的值 */
#define ALINX_LED_ON      1
/* LED熄灭时输入的值 */
#define ALINX_LED_OFF     0

  
/* 把驱动代码中会用到的数据打包进设备结构体 */
struct alinx_char_dev{
	dev_t              devid;             //设备号
	struct cdev        cdev;              //字符设备
	struct class       *class;            //类
	struct device      *device;           //设备
	struct device_node *nd;               //设备树的设备节点
	int                alinx_led_gpio;    //gpio号
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
	/* 设置私有数据 */
	file_p->private_data = &alinx_char;
    printk("gpio_test module open\n");  
    return 0;  
}  
  
  
/* write函数实现, 对应到Linux系统调用函数的write函数 */  
static ssize_t gpio_leds_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)  
{  
    int retvalue;
    unsigned char databuf[1];  
	/* 获取私有数据 */
	struct alinx_char_dev *dev = file_p->private_data;
  
    retvalue = copy_from_user(databuf, buf, len);  
    if(retvalue < 0) 
    {
		printk("alinx led write failed\r\n");
		return -EFAULT;
    } 
      
    if(databuf[0] == ALINX_LED_ON)
    {
		gpio_set_value(dev->alinx_led_gpio, !!0);
    }
    else if(databuf[0] == ALINX_LED_OFF)
    {
		gpio_set_value(dev->alinx_led_gpio, !!1);
    }
    else
    {
		printk("gpio_test para err\n");
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
	
	/* 获取设备节点 */
	alinx_char.nd = of_find_node_by_path("/alinxled");
	if(alinx_char.nd == NULL)
	{
		printk("alinx_char node not find\r\n");
		return -EINVAL;
	}
	else
	{
		printk("alinx_char node find\r\n");
	}
	
	/* 获取节点中gpio标号 */
	alinx_char.alinx_led_gpio = of_get_named_gpio(alinx_char.nd, "alinxled-gpios", 0);
	if(alinx_char.alinx_led_gpio < 0)
	{
		printk("can not get alinxled-gpios");
		return -EINVAL;
	}
	printk("alinxled-gpio num = %d\r\n", alinx_char.alinx_led_gpio);
	
	/* 申请gpio标号对应的引脚 */
	ret = gpio_request(alinx_char.alinx_led_gpio, "alinxled");
	if(ret != 0)
	{
		printk("can not request gpio\r\n");
	}
	
	/* 把这个io设置为输出 */
	ret = gpio_direction_output(alinx_char.alinx_led_gpio, 1);
	if(ret < 0)
	{
		printk("can not set gpio\r\n");
	}
	
	/* 注册设备号 */
	alloc_chrdev_region(&alinx_char.devid, MINOR1, DEVID_COUNT, DEVICE_NAME);
	
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
    /* 释放gpio */
    gpio_free(alinx_char.alinx_led_gpio);

    /* 注销字符设备 */
    cdev_del(&alinx_char.cdev);
    
    /* 注销设备号 */
    unregister_chrdev_region(alinx_char.devid, DEVID_COUNT);
    
    /* 删除设备节点 */
    device_destroy(alinx_char.class, alinx_char.devid);
    
    /* 删除类 */
    class_destroy(alinx_char.class);
       
    printk("gpio_led_dev_exit_ok\n");  
}  
  
/* 标记加载、卸载函数 */  
module_init(gpio_led_init);  
module_exit(gpio_led_exit);  
  
/* 驱动描述信息 */  
MODULE_AUTHOR("Alinx");  
MODULE_ALIAS("gpio_led");  
MODULE_DESCRIPTION("PINCTRL AND GPIO LED driver");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  









