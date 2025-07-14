#!/bin/sh
# /usr/bin/usb-gadget.sh — configure BBB as an RNDIS gadget + static USB0 address

set -e

G=/sys/kernel/config/usb_gadget/bbb

# 1) Load composite core + RNDIS function driver
modprobe libcomposite
modprobe usb_f_rndis || true

# 2) Mount configfs if needed
mountpoint -q /sys/kernel/config || mount -t configfs none /sys/kernel/config

# 3) Build the gadget on first run
if [ ! -d "$G" ]; then
    cd /sys/kernel/config/usb_gadget
    mkdir bbb
    cd bbb

    # Device IDs
    echo 0x0525 > idVendor
    echo 0xa4a2 > idProduct
    echo 0x0100 > bcdDevice
    echo 0x0200 > bcdUSB

    # Microsoft OS descriptors (Windows-friendly RNDIS)
    mkdir -p os_desc
    echo 1       > os_desc/use
    echo MSFT100 > os_desc/qw_sign
    echo 0x01    > os_desc/b_vendor_code

    # Descriptive strings
    mkdir -p strings/0x409
    echo "highlandBiosciencesLtd" > strings/0x409/manufacturer
    echo "00000000001"             > strings/0x409/serialnumber
    echo "frankenBeagle"           > strings/0x409/product

    # Configuration “c.1”
    mkdir -p configs/c.1/strings/0x409
    echo "RNDIS network" > configs/c.1/strings/0x409/configuration
    echo 250            > configs/c.1/MaxPower

    # RNDIS function
    mkdir -p functions/rndis.usb0
    # (optional) set device & host MACs here:
    # echo "5e:cd:f5:d3:0b:56" > functions/rndis.usb0/dev_addr
    # echo "b2:d4:60:08:6d:b3" > functions/rndis.usb0/host_addr

    ln -s functions/rndis.usb0 configs/c.1/

    # Wait for UDC to appear, then bind
    # (sometimes UDC isn’t ready instantly)
    UDC=""
    while [ -z "$UDC" ]; do
      UDC=$(ls /sys/class/udc | head -n1 || true)
      sleep 0.1
    done
    echo "$UDC" > UDC
fi

# 4) Bring up the interface & IP on every boot
ip link set dev usb0 up
ip addr flush dev usb0
ip addr add 192.168.137.2/30 dev usb0
ip route add default via 192.168.137.1 dev usb0 || true

exit 0

