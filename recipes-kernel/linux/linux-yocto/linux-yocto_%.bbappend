# Kernel patches
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# 1) Kernel configuration fragment
SRC_URI += "file://extra-configs.cfg"
KERNEL_CONFIG_FRAGMENTS += "extra-configs.cfg"

# 2) Add Kconfig & Makefile support for ft800-driver
SRC_URI += "file://0001-add-Kconfig-Makefile.patch"
# SRC_URI += "file://0002-device-tree-additions.patch"

# 3) stub driver sources — unpack into WORKDIR
SRC_URI += "\
    file://ft800-driver/ft800.c \
    file://ft800-driver/ft800.h \
    file://ft800-driver/ft800_ioctl.c \
    file://ft800-driver/Kconfig \
    file://ft800-driver/Makefile \
    file://ds1302-gpio-driver/rtc-ds1302-gpio.c \
    file://ds1302-gpio-driver/Kconfig \
    file://ds1302-gpio-driver/Makefile \
    "

# 4) Move the driver directories into the kernel tree
do_patch:prepend() {
    bbnote "Staging FT800 driver sources into ${S}/drivers/ft800-driver"
    install -d -v ${S}/drivers/ft800-driver
    cp -rv ${WORKDIR}/ft800-driver/* ${S}/drivers/ft800-driver/

    bbnote "Staging DS1302 Dallas driver sources into ${S}/drivers/ds1302-gpio-driver"
    install -d -v ${S}/drivers/ds1302-gpio-driver
    cp -rv ${WORKDIR}/ds1302-gpio-driver/* ${S}/drivers/ds1302-gpio-driver/
}


