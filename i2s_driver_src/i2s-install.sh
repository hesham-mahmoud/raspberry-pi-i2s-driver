# Set GPIO alternate functions
gpio_alt -p 18 -f 0
gpio_alt -p 19 -f 0
gpio_alt -p 20 -f 0
gpio_alt -p 21 -f 0
# Build and install I2S kernel module
if [[ -e i2s_driver.c ]] ; then
	make
else
	echo "No source found. Continuing..."
fi

if [[ -e i2s_driver.ko ]] ; then
	sudo insmod i2s_driver.ko
	MAJOR="$(grep i2s_driver /proc/devices | awk '{print $1}')"
	sudo mknod /dev/i2s c ${MAJOR} 0
	sudo chmod 666 /dev/i2s
else
	echo "i2s_driver.ko doesn't exist. Exiting."
	exit 1
fi
