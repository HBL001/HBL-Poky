#!/bin/sh
# /usr/bin/usb-gadget.sh — configure BBB as a single-function RNDIS gadget

set -e
# Bail out on any error; prevents half-configured gadget

G=/sys/kernel/config/usb_gadget/bbb
# Path where ConfigFS exposes our gadget instance

# 1) Load gadget core
modprobe libcomposite
# Must be present for any composite USB gadget

# 2) (Optional) load the RNDIS function — if built as a module
#    If you built CONFIG_USB_CONFIGFS_RNDIS=y, this may not be needed.
modprobe usb_f_rndis || true

# 3) Mount configfs if not already
mountpoint -q /sys/kernel/config || mount -t configfs none /sys/kernel/config

# 4) Exit if gadget already exists
[ -d "$G" ] && exit 0

# 5) Create top-level gadget directory
cd /sys/kernel/config/usb_gadget
mkdir -p bbb
cd bbb

# 6) Write device IDs
echo 0x0525 > idVendor    # “dummy” but Windows-friendly Vendor ID
echo 0xa4a2 > idProduct   # “dummy” but Windows-friendly Product ID
echo 0x0100 > bcdDevice   # device release number 1.00
echo 0x0200 > bcdUSB      # USB specification version 2.00

# 7) Microsoft OS descriptor, to auto-install RNDIS on Windows hosts
mkdir -p os_desc
echo 1       > os_desc/use
echo MSFT100 > os_desc/qw_sign
echo 0x01    > os_desc/b_vendor_code

# 8) Human-readable strings (en-US, 0x409)
mkdir -p strings/0x409
echo "BeagleBoard.org"   > strings/0x409/manufacturer
echo "BBGadget0001"      > strings/0x409/serialnumber
echo "BeagleBone Black"  > strings/0x409/product

# 9) One configuration with a single RNDIS function
mkdir -p configs/c.1
echo 250 > configs/c.1/MaxPower
#    250mA maximum current draw

# 10) Ensure functions/ exists, then create rndis.usb0
mkdir -p functions
mkdir    functions/rndis.usb0
#   This directory comes from CONFIG_USB_CONFIGFS_RNDIS=y

# 11) Bind the RNDIS function into our configuration
ln -s functions/rndis.usb0 configs/c.1/

# 12) Activate the gadget by writing the UDC name
echo "$(ls /sys/class/udc | head -n1)" > UDC

# 13) Bring up the network interface
ip link set dev usb0 up
ip addr add 192.168.7.2/30 dev usb0
#    Uses a /30 netmask so only .1 (host) and .2 (device) exist

exit 0

