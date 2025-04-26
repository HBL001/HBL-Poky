#!/bin/sh
#
# /usr/bin/usb-gadget.sh — configure BBB as a single-function RNDIS gadget
#

set -e
G=/sys/kernel/config/usb_gadget/bbb

# Load composite core + RNDIS function driver
modprobe libcomposite
modprobe usb_f_rndis

# Mount configfs if it isn’t already
mountpoint -q /sys/kernel/config || mount -t configfs none /sys/kernel/config

# If it’s already set up, exit
[ -d "$G" ] && exit 0

cd /sys/kernel/config/usb_gadget

# Create gadget directory (no-op if already exists)
mkdir -p bbb
cd bbb

# Device descriptors
echo 0x0525 > idVendor
echo 0xa4a2 > idProduct
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB

# Microsoft OS descriptors for RNDIS
mkdir -p os_desc
echo 1       > os_desc/use
echo MSFT100 > os_desc/qw_sign
echo 0x01    > os_desc/b_vendor_code

# Strings: language = en-US (0x409)
mkdir -p strings/0x409
echo "BeagleBoard.org"   > strings/0x409/manufacturer
echo "BBGadget0001"      > strings/0x409/serialnumber
echo "BeagleBone Black"  > strings/0x409/product

# One configuration with RNDIS
mkdir -p configs/c.1
echo 250 > configs/c.1/MaxPower

# Create and bind the RNDIS function
# (now succeeds because usb_f_rndis is loaded)
mkdir -p functions
mkdir functions/rndis.usb0
ln -s functions/rndis.usb0 configs/c.1/

# Bind to the first UDC (USB controller)
echo "$(ls /sys/class/udc | head -n1)" > UDC

# Bring up the network interface
ip link set dev usb0 up
ip addr add 192.168.7.2/30 dev usb0

exit 0

