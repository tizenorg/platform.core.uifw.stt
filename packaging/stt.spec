Name:       stt
Summary:    Speech To Text client library and daemon
Version:    0.1.41
Release:    1
Group:      UI Framework/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(capi-media-audio-io)
BuildRequires:  pkgconfig(capi-media-sound-manager)

BuildRequires:  cmake

%description
Speech To Text client library and daemon.


%package devel
Summary:    Speech To Text header files for STT development
Group:      Development/UI Framework
Requires:   %{name} = %{version}-%{release}

%description devel
Speech To Text header files for STT development.


%prep
%setup -q 


%build
%cmake . 
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest stt-server.manifest
%license LICENSE.APLv2
/opt/etc/smack/accesses.d/stt-server.rule
/etc/config/sysinfo-stt.xml
%defattr(-,root,root,-)
%{_libdir}/libstt.so
%{_libdir}/libstt_setting.so
%{_prefix}/lib/voice/stt/1.0/sttd.conf
%{_bindir}/stt-daemon

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/stt.pc
%{_libdir}/pkgconfig/stt-setting.pc
%{_includedir}/stt.h
%{_includedir}/stt_setting.h
%{_includedir}/sttp.h
