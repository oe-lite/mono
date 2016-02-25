DESCRIPTION = "Mono is an open source implementation of Microsoft's .NET Framework based on the ECMA standards for C# and the Common Language Runtime."
LICENSE = "LGPL-2.0"
RECIPE_TYPES = "native machine"

inherit auto-package-libs autotools c++

# make the machine context depend on its native counter part to
# enable bootstrapping
DEPENDS = "libm libpthread librt libdl"
DEPENDS:>machine = " native:mono native:mono-jay"
SRC_URI = "http://download.mono-project.com/sources/mono/mono-${PV}.${SV}.tar.bz2"
SRC_URI:>machine = " file://enable-cross-mcs-build.patch"

# enable X11 if configured so by distro/machine
RECIPE_FLAGS = "x11"
MONO_X = "--with-x=no"
MONO_X:USE_x11 = "--with-x=yes"
DEPENDS:>USE_x11 = " libx11"

# comments to configure flags:
#   tls (thread local storage) fails to build with pthread
#   static_mono=no fails to build
EXTRA_OECONF = " \
    --enable-shared \
    --disable-rpath \
    --disable-system-aot \
    --with-gnu-ld \
    --with-tls=__thread \
    --with-static_mono=yes \
    --with-shared_mono=yes \
    --with-xen_opt=no \
    --with-mcs-docs=no \
    --with-sigaltstack=yes \
    ${MONO_X} \
"
EXTRA_OECONF:>native = " --disable-boehm"
EXTRA_OECONF:>machine = " --enable-mcs-build"

# the cross build should use the native version of the
# 'jay' binary, so fix the makefiles
DO_CONFIGURE_JAY = ""
DO_CONFIGURE_JAY:machine = "do_configure_jay"
do_configure[postfuncs] += "${DO_CONFIGURE_JAY}"
do_configure_jay() {
    find ${S} -name Makefile -exec sed -ir 's![^ \t]*jay/jay!jay!' {} \;
}

# fix cross build calls to mono
DO_CONFIGURE_LIBTOOL = ""
DO_CONFIGURE_LIBTOOL:machine = "do_configure_libtool"
do_configure[postfuncs] += "${DO_CONFIGURE_LIBTOOL}"
do_configure_libtool() {
    # make the wrapper use the native mono binary instead of libtooling it
    cat <<EOF > ${S}/runtime/mono-wrapper
#!/bin/sh
${BUILD_SYSROOT}/bin/mono --config ${S}/runtime/etc/mono/config "\$@"
EOF
}

# configure make to use native mcs and mono binaries
EXTRA_OEMAKE:machine = " \
    EXTERNAL_MCS=${BUILD_SYSROOT}/bin/mcs \
    EXTERNAL_RUNTIME=${BUILD_SYSROOT}/bin/mono \
    V=1 \
"

DO_INSTALL_FUNC = "do_install_make"
DO_INSTALL_FUNC:native  = "do_install_native"
do_install() {
    ${DO_INSTALL_FUNC}
}

# partly rebuilds cannot find its own binaries, so
# help it out by updating the path
do_install_native() {
    export MONO_PATH=${S}/mcs/class/lib/basic
    export PATH=$PATH:${S}/runtime/_tmpinst/bin/
    do_install_make
}

# fix the mcs wrapper script to use the relative mono and
# mcs binaries from the staging dir
DO_INSTALL_MCS = ""
DO_INSTALL_MCS:native = "do_install_mcs"
do_install[postfuncs] += "${DO_INSTALL_MCS}"
do_install_mcs() {
    sed -i ${D}${bindir}/mcs \
        -e 's!/bin/mono!mono!' \
        -re 's!(/lib/mono/4.5/mcs.exe)!$(dirname $0)/..\1!'
}

FILES_${PN} += "${libdir} ${datadir}"
DEPENDS_${PN} += "libm libdl libpthread libc libgcc librt libstdc++"
RDEPENDS_${PN} += " \
    libm \
    libdl \
    libpthread \
    libc \
    libgcc-s \
    librt \
    libstdc++ \
    libmono-2.0 \
    libmonosgen-2.0 \
    libmonosupportw \
    libmonoposixhelper \
    libikvm-native \
"

AUTO_PACKAGE_LIBS = " \
    mono-2.0 \
    monosgen-2.0 \
    mono-profiler-iomap \
    mono-profiler-log \
    mono-profiler-aot \
    monosupportw \
    ikvm-native \
    monoposixhelper \
"
AUTO_PACKAGE_LIBS:>machine = " monoboehm-2.0"
AUTO_PACKAGE_LIBS_DEPENDS = "libc libdl libgcc libm libpthread librt libstdc++"
AUTO_PACKAGE_LIBS_RDEPENDS = "libc libdl libgcc libm libpthread librt libstdc++"

# libmono-2.0
PROVIDES_${PN}-libmono-2.0[qa] += "allow-missing-soname:libmono-2.0"
DEPENDS_${PN}-libmono-2.0[qa] += "${PN}-libmonosgen-2.0"
FILES_${PN}-libmono-2.0-dev += "${libdir}/pkgconfig/mono-2.pc"

# libmonosgen-2.0
PROVIDES_${PN}-libmonosgen-2.0 += "libmonosgen-2.0-1"
DEPENDS_${PN}-libmonosgen-2.0[qa] += "allow-missing-lib-depends"
FILES_${PN}-libmonosgen-2.0-dev += "${libdir}/pkgconfig/monosgen-2.pc"

# libmonoboehm-2.0
PROVIDES_${PN}-libmonoboehm-2.0 += "libmonoboehm-2.0-1"

# libmono-profiler-iomap
PROVIDES_${PN}-libmono-profiler-iomap += "libmono-profiler-iomap-0"
DEPENDS_${PN}-libmono-profiler-iomap += "${PN}-libmonosgen-2.0"
RDEPENDS_${PN}-libmono-profiler-iomap += "${PN}-libmonosgen-2.0"

# libmono-profiler-log
PROVIDES_${PN}-libmono-profiler-log += "libmono-profiler-log-0"
DEPENDS_${PN}-libmono-profiler-log += "libmonosgen-2.0"
RDEPENDS_${PN}-libmono-profiler-log += "libmonosgen-2.0"
FILES_${PN}-libmono-profiler-log += "${libdir}/libmono-profiler-log-static.a ${libdir}/libmono-profiler-log-static.la"

# libmono-profiler-aot
PROVIDES_${PN}-libmono-profiler-aot += "libmono-profiler-aot-0"
DEPENDS_${PN}-libmono-profiler-aot += "libmonosgen-2.0"
RDEPENDS_${PN}-libmono-profiler-aot += "libmonosgen-2.0"

# libmonosupportw
PROVIDES_${PN}-libmonosupportw[qa] += "allow-missing-soname:libmonosupportw"
FILES_${PN}-libmonosupportw += "${libdir}/libMonoSupportW.so"
FILES_${PN}-libmonosupportw-dev += "${libdir}/libMonoSupportW.la ${libdir}/libMonoSupportW.a"

# libikvm-native
PROVIDES_${PN}-libikvm-native[qa] += "allow-missing-soname:libikvm-native"
FILES_${PN}-libikvm-native += "${libdir}/libikvm-native.so"

# libmonoposixhelper
FILES_${PN}-libmonoposixhelper = "${libdir}/libMonoPosixHelper.so"
FILES_${PN}-libmonoposixhelper-dev = "${libdir}/libMonoPosixHelper.a ${libdir}/libMonoPosixHelper.la"

# create a native package for the jay binary and
# install that binary into it
PACKAGES:<native = "${PN}-jay "
FILES_${PN}-jay:native = "${bindir}/jay"
DO_INSTALL_JAY = ""
DO_INSTALL_JAY:native = "do_install_jay"
do_install[postfuncs] += "${DO_INSTALL_JAY}"
do_install_jay() {
    install -d ${D}${bindir}
    install -m0755 ${S}/mcs/jay/jay ${D}${bindir}/jay
}
