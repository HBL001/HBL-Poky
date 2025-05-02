FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://user-sudoers"

do_install:append() {
    install -d ${D}${sysconfdir}/sudoers.d
    install -m 0440 ${WORKDIR}/user-sudoers ${D}${sysconfdir}/sudoers.d/user
}

FILES:${PN} += "${sysconfdir}/sudoers.d"

