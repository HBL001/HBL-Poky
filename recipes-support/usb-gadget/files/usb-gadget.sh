#!/bin/sh
# /usr/bin/usb-gadget.sh — configure BBB as an RNDIS gadget + static USB0 address

set -e

G=/sys/kernel/config/usb_gadget/bbb

# 1) Load composite core + RNDIS function driver (built-in via CONFIG_USB_CONFIGFS_RNDIS=y)
modprobe libcomposite
modprobe usb_f_rndis || true

# 2) Mount configfs if needed
mountpoint -q /sys/kernel/config || mount -t configfs none /sys/kernel/config

# 3) If we’ve never configured the gadget, do so now
if [ ! -d "$G" ]; then
    cd /sys/kernel/config/usb_gadget
    mkdir -p bbb && cd bbb

    # Device IDs (you can swap these for 1d6b:0104 if you prefer Linux Foundation defaults)
    echo 0x0525 > idVendor
    echo 0xa4a2 > idProduct
    echo 0x0100 > bcdDevice
    echo 0x0200 > bcdUSB

    # Microsoft OS descriptors (force RNDIS driver install on Windows)
    mkdir -p os_desc
    echo 1       > os_desc/use
    echo MSFT100 > os_desc/qw_sign
    echo 0x01    > os_desc/b_vendor_code

    # Human-readable strings (en-US)
    mkdir -p strings/0x409
    echo "highlandBiosciencesLtd"     > strings/0x409/manufacturer
    echo "00000000001"        > strings/0x409/serialnumber
    echo "frankenBeagle" > strings/0x409/product

    # Single RNDIS configuration
    mkdir -p configs/c.1
    echo 250 > configs/c.1/MaxPower

    # Create RNDIS function and bind
    mkdir -p functions
    mkdir    functions/rndis.usb0
    ln -s functions/rndis.usb0 configs/c.1/

    # Activate the gadget
    echo "$(ls /sys/class/udc | head -n1)" > UDC
fi

# 4) Bring up the USB network interface and assign the static /30 address
ip link set dev usb0 up
ip addr add 192.168.7.2/30 dev usb0 || true
ip route add default via 192.168.7.1 dev usb0


exit 0

