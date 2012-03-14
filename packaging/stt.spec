Name:       stt
Summary:    Speech To Text client library and daemon
Version:    0.1.1
Release:    1
Group:      libs
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(openssl)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(mm-camcorder)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)

BuildRequires:  cmake

%description
Speech To Text client library and daemon.


%package devel
Summary:    Speech To Text header files for STT development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description devel
Speech To Text header files for STT development.


%prep
%setup -q -n %{name}-%{version}


%build
cmake . -DCMAKE_INSTALL_PREFIX=/usr
make %{?jobs:-j%jobs}

%install
%make_install




%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig





%files
%{_libdir}/libstt.so
%{_libdir}/libstt_setting.so
%{_bindir}/stt-daemon


%files devel
%{_libdir}/pkgconfig/stt.pc
%{_libdir}/pkgconfig/stt-setting.pc
%{_includedir}/stt.h
%{_includedir}/stt_setting.h
%{_includedir}/sttp.h
