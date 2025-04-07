SUMMARY = "CMSIS-NN: Efficient kernels for Arm Cortex-M and Cortex-A processors"
DESCRIPTION = "CMSIS-NN provides optimized neural network kernels to maximize performance and minimize memory footprint."
HOMEPAGE = "https://github.com/ARM-software/CMSIS-NN"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SRC_URI = "git://github.com/ARM-software/CMSIS-NN.git;branch=main;protocol=https"
SRCREV = "e096196a0c49f065abc03d943c583cd50de424ba" 

S = "${WORKDIR}/git"
B = "${WORKDIR}/build"

inherit cmake

EXTRA_OECMAKE += "-DCMSIS_PATH=${S} -DCMAKE_POSITION_INDEPENDENT_CODE=ON"

do_install() {
    #install -d ${D}${libdir}
    #install -m 0644 ${B}/libcmsis-nn.a ${D}${libdir}/

    #install -d ${D}${includedir}/cmsis-nn
    #cp -r ${S}/Include/* ${D}${includedir}/cmsis-nn/

  # Install static library
    install -d ${D}${libdir}
    if [ -f ${B}/libcmsis-nn.a ]; then
        install -m 0644 ${B}/libcmsis-nn.a ${D}${libdir}/
    else
        bbwarn "libcmsis-nn.a not found in ${B}"
    fi

    # Install headers
    if [ -d ${S}/Include ]; then
        install -d ${D}${includedir}/cmsis-nn
        cp -r ${S}/Include/* ${D}${includedir}/cmsis-nn/
    else
        bbwarn "No headers found at ${S}/Include"
    fi
}

