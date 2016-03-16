DESCRIPTION = "Test applications for Mono"

DEPENDS = "native:mono"
SRC_URI = "git://github.com/netroby/mono-helloworld.git;protocol=https;${SRC_REV}"
S = "${SRCDIR}/mono-helloworld"
FILES_${PN} = "${bindir}/hello.exe"
RDEPENDS_${PN} = "mono"

do_compile() {
    mcs hello.cs
}

do_install() {
    install -d ${D}${bindir}
    install ${S}/hello.exe ${D}${bindir}/hello.exe
}
