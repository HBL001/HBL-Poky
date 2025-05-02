FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Kernel patches
SRC_URI += "file://0001-Remove-HDMI-Includes.patch"
# Add other patches as needed
#SRC_URI += "file://0002-Add-SPI0-ready-for-FT843.patch"
#SRC_URI += "file://0003-Add-I2C1-and-SPI-Frequency-Limit.patch"
#SRC_URI += "file://0004-Add-IRQ-7-TPS62517-PMIC.patch"
#SRC_URI += "file://0005-Add-DS1302-GPIO-DRIVER.patch"
#SRC_URI += "file://0006-invert-sdcard.patch"

# Kernel configuration fragment
SRC_URI += "file://kernel.cfg"


# FT800 stub driver source files
SRC_URI += "file://ft800-driver/ft800.c \
            file://ft800-driver/Makefile \
            file://ft800-driver/Kconfig"

# Inject FT800 driver into kernel source tree
do_configure:append() {

    # Create destination directory in kernel tree
    mkdir -p ${S}/drivers/ft800-driver

    # Copy all FT800 driver files into kernel source tree
    cp ${WORKDIR}/ft800-driver/* ${S}/drivers/ft800-driver/

    # Register driver in the kernel build system
    echo 'obj-y += ft800-driver/' >> ${S}/drivers/Makefile
    echo 'source "drivers/ft800-driver/Kconfig"' >> ${S}/drivers/Kconfig
}


