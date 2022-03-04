# Shared object major library version
%global libsomajor 0

Name:           unnks
Version:        0.2.5
Release:        0%{?dist}
Summary:        nks and nkx archive unpacker

License:        GPLv3+
URL:            https://github.com/JimiHFord/unnks
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  autoconf
BuildRequires:  autoconf-archive
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  libgcrypt-devel
BuildRequires:  pkgconfig(glib-2.0)
Requires:       %{name}-libs%{?_isa} = %{version}-%{release}

%description
unnks extracts data from nks and nkx archives,
which are commonly used by several music synthesis programs.

%package        libs
Summary:        Libraries for %{name}

%description    libs
The %{name}-libs package contains libraries for running applications
that use %{name}.

%package        devel
Summary:        Development files for %{name}
Requires:       %{name}-libs%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%prep
%autosetup


%build
autoreconf -fiv
%configure
%make_build


%install
%make_install

# Purge libtool archive files
rm -rf %{buildroot}%{_libdir}/*.la

# Purge static libraries
rm -rf %{buildroot}%{_libdir}/*.a

# We'll grab these files ourselves later
rm -rf %{buildroot}%{_docdir}/%{name}


%ldconfig_scriptlets libs

%files
%license GPL3 COPYING LICENSE
%doc README.md AUTHORS
%{_bindir}/nks-scan
%{_bindir}/unnks

%files libs
%license GPL3 COPYING LICENSE
%{_libdir}/libnks.so.%{libsomajor}{,.*}

%files devel
%{_includedir}/nks.h
%{_libdir}/libnks.so


%changelog

