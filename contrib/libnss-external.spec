Name:           libnss-external
Version:        1.0
Release:        1%{?dist}
Summary:        NSS library to provide NSS db entries from external commands.
Group:          System Environment/Libraries
License:        No License
URL:            https://github.com/sbalneav/%{name}
Source0:        https://github.com/sbalneav/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: make

%description
libnss_external is an nss library designed to provide nss services using the text output of commands. It currently implements the passwd, group, and shadow databases for lookup.

%package devel
Summary: Headers for building apps that use libnss-external
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
Headers for building apps that use libnss-external

%prep
%setup -q

%build
./autogen.sh
%configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%files
%{_libdir}/libnss_external.so*
%{_mandir}/man5/nss_external.5.gz

%files devel
%{_libdir}/libnss_external.a
%{_libdir}/libnss_external.la

%changelog
* Mon Jul 16 2018 Matthew Paine <matt@mattsoftware.com> - 1.0-1
- Initial spec file

