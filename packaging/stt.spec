Name:       stt
Summary:    Speech To Text client library and daemon
Version:    0.1.1
Release:    1
Group:      libs
License:    Samsung
Source0:    stt-0.1.1.tar.gz
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
BuildRequires:  pkgconfig(dnet)
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
rm -rf %{buildroot}
%make_install




%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig





%files
%defattr(-,root,root,-)
%{_libdir}/libstt.so
%{_libdir}/libstt_setting.so
%{_bindir}/stt-daemon


%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/stt.pc
%{_libdir}/pkgconfig/stt-setting.pc
%{_includedir}/stt.h
%{_includedir}/stt_setting.h
%{_includedir}/sttp.h
