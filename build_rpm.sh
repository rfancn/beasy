#!/bin/sh
#
# Script for building a binary and source rpm. 

# The package name.
PACKAGE=beasy

# The current version.
VERSION=0.2

# Path to temporary storage.
if [ "$TMP" = "" ]
then
    TMP=/tmp
fi    

# Create a temporary local RPM build environment.
RPMBUILD_TMPDIR="rpm"`echo $$`
mkdir -p --verbose $TMP/$RPMBUILD_TMPDIR/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
mkdir -p --verbose $TMP/$RPMBUILD_TMPDIR/RPMS/{athlon,i386,i486,i586,i686,noarch}
mv -f ~/.rpmmacros ~/.rpmmacros.save
echo "%_topdir $TMP/$RPMBUILD_TMPDIR" > ~/.rpmmacros

# Generate a distribution tarball.
./configure
make dist

PACKAGE_NAME=`echo beasy*.tar.gz`
if test -n $PACKAGE_NAME; then 
	# Build the RPMs.
	rpmbuild -ta --clean $PACKAGE_NAME
fi

# Put the RPMs in this directory.
mv -f --verbose $TMP/$RPMBUILD_TMPDIR/RPMS/athlon/* . 2>/dev/null
mv -f --verbose $TMP/$RPMBUILD_TMPDIR/RPMS/i386/* .   2>/dev/null
mv -f --verbose $TMP/$RPMBUILD_TMPDIR/RPMS/i486/* .   2>/dev/null
mv -f --verbose $TMP/$RPMBUILD_TMPDIR/RPMS/i586/* .   2>/dev/null
mv -f --verbose $TMP/$RPMBUILD_TMPDIR/RPMS/i686/* .   2>/dev/null
mv -f --verbose $TMP/$RPMBUILD_TMPDIR/RPMS/noarch/* . 2>/dev/null
mv -f --verbose $TMP/$RPMBUILD_TMPDIR/SRPMS/* .       2>/dev/null

# Remove the RPM build environment.
rm -fr --verbose $TMP/$RPMBUILD_TMPDIR
rm -f --verbose $PACKAGE_NAME
rm -f ~/.rpmmacros
mv -f ~/.rpmmacros.save ~/.rpmmacros

echo
echo "********************************************************************"
echo "If you have just built a package for an architecture that was not"
echo "already available, then please email it to me <ryan.fan@oracle.com>"
exit 0
