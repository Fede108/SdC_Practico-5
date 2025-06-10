obj-m += drv.o 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dts:
	dtc -@ -I dts -O dtb -o dt_gpio.dtbo dt_gpio.dts

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	
