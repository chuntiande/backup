#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
//#include <linux/stdlib.h>
//#include <linux/stdio.h>
#include <linux/delay.h>
//API for device_node
#include <linux/fs.h>
//API for libgpio
#include <linux/gpio.h>
//API for device tree
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/string.h>

#define IIC_WR  0
#define IIC_RD  1
#define UB948   "_UB948"
#define UB947   "_UB947"
#define UH947   "_UH947"
#define UB949   "_UB949"
#define DEVICE_NAME "i2crdreg"
#define SLAVE_ADDRESS 0x0c

//GPIO0-3,GPIO5-8相应的状态寄存器地址，以及设置为输入
#define GPIO_0357_STATUS_ADDR 0x1c
#define GPIO_8_STATUS_ADDR 0x1d

//GPIO0-3,GPIO5-8设置为输出模式且拉高的值
#define GPIO0_IN_VAL   0x2a
#define GPIO1_2_IN_VAL 0x99
#define GPIO3_IN_VAL   0x09
#define GPIO5_6_IN_VAL 0x99
#define GPIO7_8_IN_VAL 0x99

//GPIO0-3,GPIO5-8设置为输入模式的寄存器地址
#define GPIO0_IN_ADDR   0x0d
#define GPIO1_2_IN_ADDR 0x0e
#define GPIO3_IN_ADDR   0x0f
#define GPIO5_6_IN_ADDR 0x10
#define GPIO7_8_IN_ADDR 0x11
//GPIO0-3,GPIO5-8设置为输入模式的值
#define GPIO0_IN_VAL   0x23
#define GPIO1_2_IN_VAL 0x33
#define GPIO3_IN_VAL   0x03
#define GPIO5_6_IN_VAL 0x33
#define GPIO7_8_IN_VAL 0x33

//判断OLDI接口是单像素还是双像素模式，再读取OpenLDI的输入视频频率
#define PF_ADDR 0x5f
#define PIXEL_MODE_ADDR 0x4f

//link状态检测连接是否断开的寄存器地址
#define LINK_STATUS_ADDR 0x0c

//读取开机寄存器复位的寄存器地址，以及开机寄存器复位的值
#define RESET_REG_ADDR   0x01
#define RESET_REG_VALUE  0x02

//不同芯片型号
#define CHIP0_ADDR 0xf0
#define CHIP1_ADDR 0xf1
#define CHIP2_ADDR 0xf2
#define CHIP3_ADDR 0xf3
#define CHIP4_ADDR 0xf4
#define CHIP5_ADDR 0xf5

//设置IIC直通使能相应的寄存器地址，设置相应的值
#define IIC_PASSTHROUGH_ADDR 0x03
#define IIC_PASSTHROUGH_VALUE 0xDA
//设置Dual双通道，双绞线模式相应的寄存器地址，以及值
#define DUAL_REG_ADDR 0x5b
#define DUAL_REG_VAL  0x23

static struct i2c_client *client;
static int i2cread_regs_8(struct i2c_client *client);
static int i2cwrite_regs_8(struct i2c_client *client);

//写寄存器函数:将GPIO输出高电平
static int set_out(struct i2c_client *client);
//十六进制转换为字符串
static void hex_to_str(char *hex, int hex_len, char *str);

static int set_iis_diable(struct i2c_client *client);
static int set_pdb_enable(struct i2c_client *client);

static int i2crdreg_open(struct inode *inode, struct file *file)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
} 

static int i2crdreg_close(struct inode *inode, struct file *file)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

static ssize_t i2crdreg_read(struct file *filp, const char __user *buf,
					size_t count, loff_t *ppos)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

static ssize_t i2crdreg_write(struct file *filp, const char __user *buf,
					size_t count, loff_t *ppos)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

static struct file_operations i2crdreg_fops = {
	.owner   =   THIS_MODULE,
	.open    =   i2crdreg_open,
	.release =   i2crdreg_close,
	.write   =   i2crdreg_write,
	.read    =   i2crdreg_read
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &i2crdreg_fops
};

//读寄存器函数
static int i2cread_regs_8(struct i2c_client *client)
{
	int ret = 0;
	u8 buf[1];
    int len;
	u8 value[] = {0};
    u8 bin_arr[] = {0};
	int i = 0;
    int m, k, j;
    u8 video_frequency = 0;
	//u8 reg[] = {0x0d, 0x0e, 0x0f, 0x10, 0x11};
	//相应寄存器的地址
	u8 reg[] = {
        GPIO_0357_STATUS_ADDR, GPIO_8_STATUS_ADDR, 
        PIXEL_MODE_ADDR, PF_ADDR, RESET_REG_ADDR, 
        IIC_PASSTHROUGH_ADDR, DUAL_REG_ADDR, CHIP0_ADDR, 
        CHIP1_ADDR, CHIP2_ADDR, CHIP3_ADDR, CHIP4_ADDR, 
        CHIP5_ADDR, LINK_STATUS_ADDR
    };
	//client->addr = 0x0c;
	u32 paddr = 0x0c;
	struct i2c_msg msg[2];
	u8 addr;
	addr = paddr&0xff;
    len = sizeof(reg);
	printk(KERN_CRIT"\n This is %s ", __func__);

	for(i = 0; i < len; i++)
	{
		buf[i] = reg[i];
		msg[0].addr  = addr;
		msg[0].flags = client->flags;   //flags为0，是写，表示buf是我们要发送的数据
		msg[0].buf   = buf;             //寄存器地址
		msg[0].len   = sizeof(buf);     //len为buf的大小，单位是字节

		msg[1].addr  = addr;
		msg[1].flags = client->flags | IIC_RD;   //flags为1，是读，表示buf是我们要接收的数据
		msg[1].buf   = buf;          //寄存器的值
		msg[1].len   = 1;
		ret = i2c_transfer(client->adapter, msg, 2);
		if(ret < 0)
		{
			printk(KERN_CRIT"read regs from i2c has been failed, ret = %d \r\n", ret);
			return ret;
		}
		value[i] = buf[i];
		printk(KERN_CRIT"\n hex  default regadd: 0x%02x---value is 0x%02x", reg[i], value[i]);
        switch(i) {
            case 0: printk(KERN_CRIT"GPIO0357 status:");break;
            case 1: printk(KERN_CRIT"GPIO8    status:");break;
            case 2: 
                printk(KERN_CRIT"to judge if it is dual or single mode\n");
                for(j = 0;j < 8; j++)
                {
                    m = value[i]%2;  //取2的余数
                    k = value[i]/2;  //取被2整除的结果
                    value[i] = k;
                    bin_arr[j] = m;    //将余数存入数组bin_arr数组中
                    printk(KERN_CONT"%d", bin_arr[j]);
                }
                break;
            case 3: 
                printk(KERN_CRIT"Read the video_frequency");
                if(bin_arr[1] == 0)
                {
                    video_frequency  = value[i] / 2;
                    printk(KERN_CRIT"bit 6 is %d, and it detect the mode is Dual-pixel, so the video_frequency of OLDI is %d", bin_arr[1], video_frequency);
                }
                else if(bin_arr[1] == 1)
                {
                    video_frequency  = value[i];
                    printk(KERN_CRIT"bit 6 is %d, and it detect the mode is Single-pixel, so the video_frequency of OLDI is %d", bin_arr[1], video_frequency);
                }
                break;
            case 4: printk(KERN_CRIT"judge if the reg is set reset");break;
            case 5: printk(KERN_CRIT"To set IIC pass through");break;
            case 6: printk(KERN_CRIT"To set dual pair");break;
           
            case 7: printk(KERN_CRIT"CHIP0  %c", value[i]); break;
            case 8: printk(KERN_CRIT"CHIP1  %c", value[i]); break;
            case 9: printk(KERN_CRIT"CHIP2  %c", value[i]); break;
            case 10: printk(KERN_CRIT"CHIP3  %c", value[i]);break;
            case 11: printk(KERN_CRIT"CHIP4  %c", value[i]);break;
            case 12: printk(KERN_CRIT"CHIP5  %c", value[i]);break;

            case 13: 
                printk(KERN_CRIT"Link_status");
                if(value[i] == 5)
                {
                    printk(KERN_CRIT"link is detected,  link_status = 1");
                }
                else printk(KERN_CRIT"link is not detected, link_status is 0");
                break;
            default: printk(KERN_CRIT"error");break;
        }
	}
	return 0;
}

//十六进制转换为字符串
static void hex_to_str(char *hex, int hex_len, char *str)
{
	int i, pos = 0;
	for(i = 0;i < hex_len; i++)
	{
		printk(str+pos,"%02x", hex[i]);
		pos+=2;
	}
}

//写寄存器函数:将GPIO输出高电平
static int set_out(struct i2c_client *client)
{
	int ret;
	int i = 0;
	u8 buf[2];
	//client->addr = 0x0c;
	u32 paddr = 0x0c;
	struct i2c_msg msg;
	u8 addr;
	//GPIO0-3,GPIO5-8相应的寄存器地址，增加使能IIC的寄存器地址0x03,设置寄存器复位芯片相应的寄存器地址, 设置Dual双通道，双绞线模式相应的寄存器地址
	u8 addr_arr[] = {0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x03, 0x01, 0x5b, };
	//GPIO0-3,GPIO5-8的值设置为输出且拉高，增加使能IIC的寄存器值0xDA
	u8 val_arr[]  = {0x2A, 0x99, 0x09, 0x99, 0x99, 0xDA,0x02, 0x23, }; 
	addr = paddr&0xff;
	printk(KERN_CRIT"\n This is %s ", __func__);

	for(i = 0; i < 6; i++)
	{
		u8 reg = addr_arr[i];
		u8 val = val_arr[i];
		
    	buf[0] = reg;
		buf[1] = val;
		msg.addr  = addr;
		msg.flags = client->flags;   //flags为0，是写，表示buf是我们要发送的数据
		msg.buf   = buf;             //寄存器地址和要写的地址
		msg.len   = sizeof(buf);     //len为buf的大小，单位是字节

		ret = i2c_transfer(client->adapter, &msg, 1);
		if(ret < 0)
		{
			printk(KERN_CRIT"read regs from i2c has been failed, ret = %d \r\n", ret);
			return ret;
		}
		printk(KERN_CRIT"default regadd: 0xf%d---val is 0x%02x", i, val);
	}

	return 0;
}

//写寄存器函数:设置IIS功能关闭
static int set_iis_diable(struct i2c_client *client)
{
	int ret;
	u8 buf[2];
	int i = 0;
	//client->addr = 0x0c;
	u32 paddr = 0x0c;
	struct i2c_msg msg;
	u8 addr;
	//设置寄存器复位芯片相应的寄存器地址
	u8 reg[] = {0x54, };
	//设置bit1为复位模式，清除整个数据块
	u8 iis_disabled = {0x00, };
	addr = paddr&0xff;
	printk(KERN_CRIT"\n This is %s ", __func__);
	/*
	for(i = 0;i < ;i++)
	{

	}*/
    buf[0] = reg;
	buf[1] = iis_disabled;
	msg.addr  = addr;
	msg.flags = client->flags;   //flags为0，是写，表示buf是我们要发送的数据
	msg.buf   = buf;             //寄存器地址和要写的地址
	msg.len   = sizeof(buf);     //len为buf的大小，单位是字节

	ret = i2c_transfer(client->adapter, &msg, 1);
	if(ret < 0)
	{
		printk(KERN_CRIT"read regs from i2c has been failed, ret = %d", iis_disabled);
		return ret;
	}
	printk(KERN_CRIT"default regadd: 0x%02x---val is 0x%02x", reg, iis_disabled);
	return 0;
}

//写寄存器函数
static int i2cwrite_regs_8(struct i2c_client *client)
{
	int ret;
	int i = 0;
    int len;
	u8 buf[2];
	//client->addr = 0x0c;
	u32 paddr = 0x0c;
	struct i2c_msg msg;
	u8 addr;
	//相应的寄存器地址
	u8 addr_arr[] = {RESET_REG_ADDR, GPIO0_IN_ADDR, GPIO1_2_IN_ADDR, 
    GPIO3_IN_ADDR, GPIO5_6_IN_ADDR, GPIO7_8_IN_ADDR,
    IIC_PASSTHROUGH_ADDR, DUAL_REG_ADDR 
    };
	//相应的值
	u8 val_arr[]  = {RESET_REG_VALUE, GPIO0_IN_VAL, GPIO1_2_IN_VAL, 
    GPIO3_IN_VAL, GPIO5_6_IN_VAL, GPIO7_8_IN_VAL,
    IIC_PASSTHROUGH_VALUE, DUAL_REG_VAL
    }; 
	addr = paddr&0xff;
	printk(KERN_CRIT"\n This is %s ", __func__);
    len = sizeof(addr_arr);
	for(i = 0; i < len; i++)
	{
		u8 reg = addr_arr[i];
		u8 val = val_arr[i];
		
    	buf[0] = reg;
		buf[1] = val;
		msg.addr  = addr;
		msg.flags = client->flags;   //flags为0,是写,表示buf是我们要发送的数据
		msg.buf   = buf;             //寄存器地址和要写的地址
		msg.len   = sizeof(buf);     //len为buf的大小,单位是字节

		ret = i2c_transfer(client->adapter, &msg, 1);
		if(ret < 0)
		{
			printk(KERN_CRIT"read regs from i2c has been failed, ret = %d \r\n", ret);
			return ret;
		}
		printk(KERN_CRIT"default regadd: 0x%02x---val is 0x%02x", addr_arr[i], val_arr[i]);
	}

	return 0;
}

//写寄存器函数:上电控制PDB使能
static int set_pdb_enable(struct i2c_client *client)
{
	int ret;
	int control_gpio = 0;
	struct device_node *test_device_node;

	//获得设备节点
	test_device_node = of_find_node_by_path("/con_test");
	if(test_device_node == NULL)
	{
		printk(KERN_CRIT"of_find_node_by_path is error ");
		return -1;
	}
	printk(KERN_CRIT"of_find_node_by_path is succeed");
	//使用of_get_named_gpio函数获取GPIO编号，此函数会将设备树中<&port4b 9 GPIO_ACTIVE_HIGH>的属性信息转换为对应的GPIO编号
	control_gpio = of_get_named_gpio(test_device_node, "control-gpio", 0);
	if(control_gpio < 0)
	{
		printk(KERN_CRIT"of_get_named_gpio is error ");
		return -1;
	}
	printk(KERN_CRIT"control_gpio is %d", control_gpio);
	//申请一个GPIO管脚
	ret = gpio_request(control_gpio, "control");
	/*if(ret < 0)
	{
		printk(KERN_CRIT"gpio_request is error");
	}*/
	printk(KERN_CRIT"gpio_request is succeed");
	//设置某个GPIO为输出，并且设置默认输出值
	gpio_direction_output(control_gpio, 0);
	printk(KERN_CRIT"gpio_direction is succeed");
	//设置输出低电平
	gpio_set_value(control_gpio, 0);
    
	return 0;
}

//与设备树的compatible相匹配
static const struct of_device_id rdreg_of_match[] = {
	{.compatible = "hsae, rdreg-i2c", 0},
	{}
};

//无设备树的时候匹配ID表
static const struct i2c_device_id rdreg_id[] = {
	{"rdreg-i2c", 0}
};

/*i2c驱动的remove函数*/
static int rdreg_remove(struct i2c_client *i2c_client, const struct i2c_device_id *id)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

/*i2c驱动的probe函数*/
static int rdreg_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	
	printk(KERN_CRIT"This is %s ", __func__);

	//设置PDB上电使能
	set_pdb_enable(client);

    i2cwrite_regs_8(client);
    i2cread_regs_8(client);
	//写寄存器函数：设置IIS功能关闭
	//set_iis_diable(client);

	//检测link状态
	//link_detect_8(client);

	//判断不同芯片的型号
	//check_chip_id(client);

    return 0;
}
//定义一个i2c_driver结构体
static struct i2c_driver rdreg_driver = {
    	.driver = {
        	.owner = THIS_MODULE,
        	.name  = "i2crdreg",
        	//采样设备树的时候驱动使用的匹配表
        	.of_match_table = rdreg_of_match,
    	},
    	.probe    = rdreg_probe,
    	.remove   = rdreg_remove,
    	.id_table = rdreg_id
};

/*驱动入口函数*/
static int rdreg_driver_init(void)
{
	int ret;
	//注册i2c_driver
	ret = i2c_add_driver(&rdreg_driver);
	if(ret < 0) {
		printk(KERN_CRIT"i2c_add_driver is error");
		return ret;
	}
	printk(KERN_CRIT"This is %s", __func__);
	return 0;
}

/*驱动出口函数*/
static void rdreg_driver_exit(void)
{
	//misc_deregister(&misc);
	//将前面注册的i2c_driver 也从linux内核中注销掉
	i2c_del_driver(&rdreg_driver);
	printk(KERN_CRIT"This is %s \n", __func__);
}

module_init(rdreg_driver_init);
module_exit(rdreg_driver_exit);
MODULE_LICENSE("GPL");


