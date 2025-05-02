FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Kernel patches
#SRC_URI += "file://0001-custom-hardware.patch"

# Kernel configuration fragment
SRC_URI += "file://kernel.cfg"

# FT800 stub driver source files
SRC_URI += "file://ft800-driver/ft800.c \
            file://ft800-driver/Makefile \
            file://ft800-driver/Kconfig \
            file://ds1302-driver/rtc-ds1302-gpio.c \
            file://ds1302-driver/Kconfig \
            file://ds1302-driver/Makefile \
            "

# Inject FT800 and DS1302 driver into kernel source tree
do_patch:append() {

    # Create destination directory in kernel tree
    mkdir -p ${S}/drivers/ft800-driver
    mkdir -p ${S}/drivers/ds1302-driver
    
    # Copy all FT800 driver files into kernel source tree
    cp ${WORKDIR}/ft800-driver/* ${S}/drivers/ft800-driver/
    cp ${WORKDIR}/ds1302-driver/* ${S}/drivers/ds1302-driver/

    # Register driver in the kernel build system
    echo 'obj-y += ft800-driver/' >> ${S}/drivers/Makefile
    echo 'source "drivers/ft800-driver/Kconfig"' >> ${S}/drivers/Kconfig
    
    # Register DS1302 in the build
    echo 'obj-$(CONFIG_RTC_DRV_DS1302_GPIO) += rtc-ds1302-gpio.o' >> ${S}/drivers/rtc/Makefile
    echo 'source "drivers/ds1302-driver/Kconfig"' >> ${S}/drivers/Kconfig
    
}

