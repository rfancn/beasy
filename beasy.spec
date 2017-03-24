Summary: Linux Support utitilies 
Name: beasy 
Version: 0.2
Release: 1 
License: GPL
Group: Utilities 
Source: %{name}-%{version}.tar.gz
Vendor: Ryan Fan <ryan.fan@oracle.com>
URL: http://wiki.us.oracle.com/calpg/SupportAndSustaining 
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: libtool, pkgconfig, gettext, glib2-devel, gtk2-devel, gstreamer-devel, libxml2-devel
Requires: glib2, gstreamer, gtk2, libxml2

%description
Bease is a series utitlities to help Linux Support.
1. It includes: the information of phone number, e.g.
time, timezone, country, state and other comments.
2. It can check personal qmon no update SR periodicaly.

%prep
%setup -q

%build
./configure --prefix=%{_prefix}
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post 

%preun 
exit 0

%files 
%defattr(-, root, root)
%{_bindir}/*
%{_datadir}/*
%{_libdir}/*

%changelog
* Fri Nov 14 2008 Ryan Fan <ryan.fan@redhat.com>
- First time to release
