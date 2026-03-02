# linux-yocto_%.bbappend
#
# Keep kernel changes reproducible: config fragment + patch series only.
# Do NOT stage driver source drops in do_patch (that causes drift vs git/patches).

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# 1) Kernel configuration fragment
SRC_URI += "file://extra-configs.cfg"
KERNEL_CONFIG_FRAGMENTS += "extra-configs.cfg"

# 2) Kernel patches (kept in sync with workspace/devtool branch)
SRC_URI += "file://0001-add-Kconfig-Makefile.patch"
SRC_URI += "file://0002-arm-dts-am335x-boneblack-add-board-specific-device-t.patch"
SRC_URI += "file://0003-drivers-add-ft800-driver.patch"
SRC_URI += "file://0004-rtc-add-ds1302-GPIO-driver.patch"
#SRC_URI += "file://0005-input-add-TPS65217-power-button-driver.patch"

# NOTE:
# We intentionally do not carry any spidev patch (not used; avoid upstream drift).
# We intentionally do not stage out-of-tree driver sources into ${S}; the patches
# already add the driver sources and wiring into the kernel tree.
