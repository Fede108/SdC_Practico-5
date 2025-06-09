cmd_/home/pi/SdC_Practico-5/gpio_driver/drv.mod := printf '%s\n'   drv.o | awk '!x[$$0]++ { print("/home/pi/SdC_Practico-5/gpio_driver/"$$0) }' > /home/pi/SdC_Practico-5/gpio_driver/drv.mod
