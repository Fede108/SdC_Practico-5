#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/uaccess.h>


#define DEVICE_NAME "rpi_gpio"
#define CLASS_NAME "my_gpio"

static dev_t devnum; 		// Global variable for the first device number
static struct cdev c_dev; 	// Global variable for the character device structure
static struct class *cl; 	// Global variable for the device class


//class_create() /* creates a class for your devices visible in /sys/class/ */
//class_destroy() /* removes the class */
//device_create() /* creates a device node in the /dev directory */
//device_destroy() /* removes a device node in the /dev directory */

/* gpios variables*/
struct gpio_desc *in0, *in1;

/*active pin*/
#define PIN_0     0
#define PIN_1     1

static int active_pin = PIN_0;



/* declarations probe and remove fuctions */
static int my_driver_probe(struct platform_device *pdev);
static int my_driver_remove(struct platform_device *pdev);

static const struct of_device_id my_drivers_ids[] = {
    { .compatible = "gpio,los-debugueadores" },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, my_drivers_ids);


static struct platform_driver my_driver = {
    .driver = {
        .name = "my_gpio_driver",
        .of_match_table = my_drivers_ids,
    },
    .probe  = my_driver_probe,
    .remove = my_driver_remove,
};


/* probe and remove fuctions */
static int my_driver_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;

    /* obtener GPIO[0] (primer pin en entrada-gpios) */
    in0 = devm_gpiod_get_index(dev, "entrada", 0, GPIOD_IN);
    if (IS_ERR(in0)) {
        dev_err(dev, "No se pudo obtener entrada 0\n");
        return PTR_ERR(in0);
    }

    /* obtener GPIO[1] (segundo pin en entrada-gpios) */
    in1 = devm_gpiod_get_index(dev, "entrada", 1, GPIOD_IN);
    if (IS_ERR(in1)) {
        dev_err(dev, "No se pudo obtener entrada 1\n");
        return PTR_ERR(in1);
    }

    dev_info(dev, "GPIOs de entrada inicializados correctamente\n");
    return 0;
}

static int my_driver_remove(struct platform_device *pdev)
{
    /* Con devm_gpiod_get_index el kernel libera todo al remover, no hace falta nada */
    return 0;
}


static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO DEVICE_NAME": close()\n");
    return 0;
}

static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Driver3_SdeC: open()\n");
    return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    int value;
    unsigned char out;

    /* solo un byte de salida */
    if (*off > 0 || len < 1)
        return 0;

    /* leer el GPIO correspondiente */
    if (active_pin == PIN_0)
        value = gpiod_get_value(in0);
    else
        value = gpiod_get_value(in1);

    /* Convertir a byte */
    out = (value != 0) ? 1 : 0;

    if (copy_to_user(buf, &out, 1))
        return -EFAULT;

    (*off)++;
    return 1;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    unsigned char user_val;

    /* esperamos 1 byte con 0 o 1 */
    if (len < 1)
        return -EINVAL;

    if (copy_from_user(&user_val, buf, 1))
        return -EFAULT;

    switch (user_val) {
        case 0:
            active_pin = PIN_0;
            break;
        case 1:
            active_pin = PIN_1;
            break;
        default:
            /* valor no válido, lo ignoramos o devolvemos error */
            return -EINVAL;
    }

    return len;
}


static struct file_operations pugs_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write
};

static int __init drv_init(void) /* Constructor */
{
    int return_value;
    struct device *dev_ret;

    if(platform_driver_register(&my_driver)) 
    {
		printk("Rpi_gpio_driver - Error! Could not load driver\n");
		return -1;
	}

    if ((return_value = alloc_chrdev_region(&devnum, 0, 1, DEVICE_NAME)) < 0)
    {
        return return_value;
    }

    if (IS_ERR(cl = class_create(THIS_MODULE, CLASS_NAME)))
    {
        unregister_chrdev_region(devnum, 1);
        return PTR_ERR(cl);
    }

    if (IS_ERR(dev_ret = device_create(cl, NULL, devnum, NULL, DEVICE_NAME)))
    {
        class_destroy(cl);
        unregister_chrdev_region(devnum, 1);
        return PTR_ERR(dev_ret);
    }

    cdev_init(&c_dev, &pugs_fops);
    if ((return_value = cdev_add(&c_dev, devnum, 1)) < 0)
    {
        device_destroy(cl, devnum);
        class_destroy(cl);
        unregister_chrdev_region(devnum, 1);
        return return_value;
    }

    printk(KERN_INFO "Rpi_gpio_driver: Registrado exitosamente..!!\n");
    return 0;
}

static void __exit drv_exit(void) /* Destructor */
{
    cdev_del(&c_dev);
    device_destroy(cl, devnum);
    class_destroy(cl);
    unregister_chrdev_region(devnum, 1);
    platform_driver_unregister(&my_driver);
    printk(KERN_INFO "Rpi_gpio_driver: dice Adios mundo cruel..!!\n");
}

module_init(drv_init);
module_exit(drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Los debugueadores");
MODULE_DESCRIPTION("Nuestro primer gpio driver");