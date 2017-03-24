#!/bin/sh
#
# [ 1.5.03 | Gunter Wambaugh ]
# Script for building a binary and source debian package (.deb). 
PACKAGE=beasy

# The architecture.
ARCH=i386

# The build/package version.
VERSION=0.2
BUILD=1

# The current directory.
CWD=`pwd`

# Path to temporary storage.
if [ "$TMP" = "" ]
then
    TMP=/tmp
fi

# Generate a distribution tarball.
./configure
make dist

# Unpack the distribution tarball.
PACKAGE_FILE=`echo "$PACKAGE"*.tar.gz`
cd $TMP
tar -xzvf $CWD/$PACKAGE_FILE

# Compile.
unpacked_dir=`ls -F /tmp | grep -e '/$' | grep $PACKAGE`
test -z $unpacked_dir && exit 1

cd $unpacked_dir
./configure \
--prefix=/usr \
--sysconfdir=/etc

make

##### Set up the package. #####
RANDOM_DIR=`echo $$`
DEB_BUILD_ROOT=$TMP/$RANDOM_DIR/$PACKAGE

prefix=/usr
confdir=/etc
datadir=$prefix/share
bindir=$prefix/bin

# Copy the executable. 
mkdir -p --verbose $DEB_BUILD_ROOT$bindir
cp -f --verbose beasy/beasy     $DEB_BUILD_ROOT$bindir
chmod --verbose 755 $DEB_BUILD_ROOT$bindir/beasy
strip $DEB_BUILD_ROOT$bindir/beasy

# Copy the pixmaps.
mkdir -p --verbose $DEB_BUILD_ROOT$datadir/beasy/pixmaps
cp -f --verbose pixmaps/* $DEB_BUILD_ROOT$datadir/beasy/pixmaps
chmod --verbose 644 $DEB_BUILD_ROOT$datadir/beasy/pixmaps/*

# Copy the icons.
mkdir -p --verbose $DEB_BUILD_ROOT$datadir/beasy/icons
cp -f --verbose icons/* $DEB_BUILD_ROOT$datadir/beasy/icons
chmod --verbose 644 $DEB_BUILD_ROOT$datadir/beasy/icons/*

# Copy the sounds.
mkdir -p --verbose $DEB_BUILD_ROOT$datadir/beasys/sounds
cp -f --verbose sounds/* $DEB_BUILD_ROOT$datadir/beasy/sounds
chmod --verbose 644 $DEB_BUILD_ROOT$datadir/beasy/sounds/*

# Copy the sounds.
mkdir -p --verbose $DEB_BUILD_ROOT$datadir/beasys/themes
cp -f --verbose themes/* $DEB_BUILD_ROOT$datadir/beasy/themes
chmod --verbose 644 $DEB_BUILD_ROOT$datadir/beasy/themes/*

# Create the package control file.
mkdir -p --verbose $DEB_BUILD_ROOT/DEBIAN
cp -f --verbose beasy.debctl $DEB_BUILD_ROOT/DEBIAN/control

##### End of package setup. #####

# Build the package:
cd $TMP/$RANDOM_DIR
fakeroot dpkg-deb --build $PACKAGE
mv -f --verbose $PACKAGE.deb $PACKAGE-$VERSION-$ARCH-$BUILD.deb
# Put the package in this directory.
mv -f --verbose $PACKAGE-$VERSION-$ARCH-$BUILD.deb $CWD

# Remove the build environment.
rm -fr --verbose $TMP/$PACKAGE_FILE
rm -fr --verbose $CWD/$PACKAGE_FILE
rm -fr --verbose $TMP/$RANDOM_DIR 

echo
echo "********************************************************************"
echo "If you have just built a package for an architecture that was not"
echo "already available, then please email it to me <ryan.fan@oracle.com>"
