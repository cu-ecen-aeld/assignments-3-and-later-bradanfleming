#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
CORES=$(nproc)

export ARCH=arm64
export CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    git reset --hard HEAD
    echo "Applying patches"
    git apply "${FINDER_APP_DIR}/dtc-multiple-definition.patch"
    echo "Cleaning kernel"
    make mrproper
    echo "Configuring kernel"
    make defconfig
    echo "Building boot image"
    make -j${CORES} Image
    echo "Building devicetree"
    make -j${CORES} dtbs
fi

echo "Adding the Image in outdir"
cp "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" "${OUTDIR}/Image"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir "${OUTDIR}/rootfs"
pushd "${OUTDIR}/rootfs"
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/lib64 usr/sbin var/log
popd

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    echo "Cloning busybox ${BUSYBOX_VERSION}"
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Cleaning busybox"
    make distclean
    echo "Configuring busybox"
    make defconfig
else
    cd busybox
fi
if [ ! -e ${OUTDIR}/busybox/busybox ]; then
	echo "Building busybox"
	make all
fi
echo "Installing busybox"
make CONFIG_PREFIX="${OUTDIR}/rootfs" install

echo "Checking library dependencies"
cd "${OUTDIR}/rootfs"
LIBC="$(dirname "$(dirname "$(which aarch64-none-linux-gnu-readelf)")")/${CROSS_COMPILE::-1}/libc"
LOADER="$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | awk 'NF>1{print substr($NF,2,length($NF)-2)}')"
LIBS="$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | awk 'NF>1{print substr($NF,2,length($NF)-2)}')"

echo "Installing library dependencies"
install -m 755 "${LIBC}/${LOADER}" "${OUTDIR}/rootfs/${LOADER}"
printf "%s" "${LIBS}" | xargs --replace=% install -m 755 "${LIBC}/lib64/%" "${OUTDIR}/rootfs/lib64/%"

echo "Installing device nodes"
sudo mknod -m 666 "${OUTDIR}/rootfs/dev/null" c 1 3
sudo mknod -m 666 "${OUTDIR}/rootfs/dev/console" c 5 1

echo "Building writer"
cd "${FINDER_APP_DIR}"
make clean
make writer

echo "Installing finder-app scripts"
install -m 755 autorun-qemu.sh "${OUTDIR}/rootfs/home/autorun-qemu.sh"
install -m 755 finder.sh "${OUTDIR}/rootfs/home/finder.sh"
install -m 755 finder-test.sh "${OUTDIR}/rootfs/home/finder-test.sh"
install -m 755 writer "${OUTDIR}/rootfs/home/writer"
install -m 755 -d "${OUTDIR}/rootfs/home/conf"
install -m 644 conf/assignment.txt "${OUTDIR}/rootfs/home/conf/assignment.txt"
install -m 644 conf/username.txt "${OUTDIR}/rootfs/home/conf/username.txt"

echo "Setting rootfs ownership"
sudo chown root:root --recursive "${OUTDIR}/rootfs"

echo "Building initramfs"
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root 2>/dev/null > "${OUTDIR}/initramfs.cpio" 2> /dev/null
gzip -f "${OUTDIR}/initramfs.cpio"

echo "Completed build in ${OUTDIR}"
