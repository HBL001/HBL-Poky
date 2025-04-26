# usb-gadget_1.0.bb — installs the RNDIS gadget script & service

DESCRIPTION = "USB RNDIS gadget auto-setup script"
# Human-readable summary of the recipe

LICENSE = "MIT"
# SPDX license identifier; must match LIC_FILES_CHKSUM below

# We have no upstream tarball, so tell BitBake to treat WORKDIR as the source
S = "${WORKDIR}"

#-----------------------------------------------------------------------------#
# FILESEXTRAPATHS: where BitBake looks for file:// URIs
#-----------------------------------------------------------------------------#
# Modern override (Yocto 3.4+); prepend our files/ directory ahead of others
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
# If you ever build on pre-3.4, you’d also add FILESEXTRAPATHS_prepend (underscore).

#-----------------------------------------------------------------------------#
# Licensing: point BitBake at our LICENSE for QA checks
#-----------------------------------------------------------------------------#
# file://LICENSE refers to meta-custom/recipes-support/usb-gadget/files/LICENSE
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8ef746efe82910ae8f240946e18eaf6"

#-----------------------------------------------------------------------------#
# SRC_URI: all of the files we ship into the image
#-----------------------------------------------------------------------------#
SRC_URI = " \
    file://LICENSE \
    file://usb-gadget.sh \
    file://usb-gadget.service \
"

#-----------------------------------------------------------------------------#
# Systemd integration
#-----------------------------------------------------------------------------#
inherit systemd
# Pull in systemd.bbclass to handle service packaging

# Tell the systemd class which .service to install & enable
SYSTEMD_SERVICE:${PN}     = "usb-gadget.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

#-----------------------------------------------------------------------------#
# do_install: copy binaries & units into the target filesystem
#-----------------------------------------------------------------------------#
do_install() {
    # 1) Create /usr/bin and install the script
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/usb-gadget.sh ${D}${bindir}/usb-gadget.sh

    # 2) Create the systemd system unit directory and install the .service
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/usb-gadget.service \
        ${D}${systemd_system_unitdir}/usb-gadget.service
}

#-----------------------------------------------------------------------------#
# FILES_${PN}: explicitly list everything to avoid QA-installed-vs-shipped errors
#-----------------------------------------------------------------------------#
# ${bindir} expands to /usr/bin
# ${systemd_system_unitdir} expands to /usr/lib/systemd/system
FILES:${PN} += " \
    ${bindir}/usb-gadget.sh \
    ${systemd_system_unitdir}/usb-gadget.service \
"

