# Kernel patches
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# 1) Kernel configuration fragment
SRC_URI += "file://extra-configs.cfg"
KERNEL_CONFIG_FRAGMENTS += "extra-configs.cfg"

# 2) Add Kconfig & Makefile support for ft800-driver
SRC_URI += "file://0001-add-ft800-Kconfig-Makefile-support.patch"
SRC_URI += "file://0002-device-tree-additions.patch"

# 3) FT800 stub driver sources — unpack into WORKDIR/ft800-driver
SRC_URI += "\
    file://ft800-driver/ft800.c \
    file://ft800-driver/ft800.h \
    file://ft800-driver/ft800_ioctl.c \
    file://ft800-driver/Kconfig \
    file://ft800-driver/Makefile \
    "

# 5) Move the entire ft800-driver directory into the kernel tree
do_patch:prepend() {
    bbnote "Staging FT800 driver sources into ${S}/drivers/ft800-driver"
    install -d -v ${S}/drivers/ft800-driver
    cp -rv ${WORKDIR}/ft800-driver/* ${S}/drivers/ft800-driver/
}

