# Get path to our extlinux.conf file relative to this append file
EXTLINUX_FILE := "${THISDIR}/files/extlinux.conf"

# Ensure kernel deploys before the WIC image is generated
do_image_wic[depends] += "virtual/kernel:do_deploy"

# Copy extlinux.conf into the target image at post-rootfs time
ROOTFS_POSTPROCESS_COMMAND:append = " inject_extlinux_conf; "

inject_extlinux_conf() {
    echo ">>> Injecting extlinux.conf from ${EXTLINUX_FILE}"

    if [ ! -f "${EXTLINUX_FILE}" ]; then
        echo "ERROR: extlinux.conf not found at ${EXTLINUX_FILE}"
        exit 1
    fi

    install -d ${IMAGE_ROOTFS}/boot/extlinux
    install -m 0644 "${EXTLINUX_FILE}" ${IMAGE_ROOTFS}/boot/extlinux/extlinux.conf

    echo ">>> extlinux.conf successfully injected to /boot/extlinux/"
}

