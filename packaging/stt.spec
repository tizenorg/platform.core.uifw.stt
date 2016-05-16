Name:       stt
Summary:    Speech To Text client library and daemon
Version:    0.2.54
Release:    1
Group:      Graphics & UI Framework/Voice Framework
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: %{name}.manifest
Source1002: %{name}-devel.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-media-audio-io)
BuildRequires:  pkgconfig(capi-media-wav-player)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(vconf)
%if "%{PRODUCT_TYPE}" == "TV"
BuildRequires:  pkgconfig(capi-network-bluetooth)
%endif

BuildRequires:  cmake

%description
Speech To Text client library and daemon.


%package devel
Summary:    Speech To Text header files for STT development
Group:      Graphics & UI Framework/Development
Requires:   %{name} = %{version}-%{release}

%package file-devel
Summary:    File To Text header files for STT FILE development
Group:      Graphics & UI Framework/Development
Requires:   %{name} = %{version}-%{release}

%package setting-devel
Summary:    Speech To Text setting header files for STT development
Group:      Graphics & UI Framework/Development
Requires:   %{name} = %{version}-%{release}

%package engine-devel
Summary:    Speech To Text engine header files for STT development
Group:      Graphics & UI Framework/Development
Requires:   %{name} = %{version}-%{release}

%description devel
Speech To Text header files for STT development.

%description file-devel
File To Text header files for STT FILE development.

%description setting-devel
Speech To Text setting header files for STT development.

%description engine-devel
Speech To Text engine header files for STT development.

%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} %{SOURCE1002} .


%build
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%if "%{PRODUCT_TYPE}" == "TV"
export CFLAGS="$CFLAGS -DTV_PRODUCT"
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
        -DTZ_SYS_RO_SHARE=%TZ_SYS_RO_SHARE -DTZ_SYS_BIN=%TZ_SYS_BIN -D_TV_PRODUCT=TRUE
%else
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
        -DTZ_SYS_RO_SHARE=%TZ_SYS_RO_SHARE -DTZ_SYS_BIN=%TZ_SYS_BIN
%endif

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{TZ_SYS_RO_SHARE}/license
install LICENSE.APLv2 %{buildroot}%{TZ_SYS_RO_SHARE}/license/%{name}

%make_install

%post 
/sbin/ldconfig

mkdir -p %{_libdir}/voice/

mkdir -p %{TZ_SYS_RO_SHARE}/voice/test


%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%license LICENSE.APLv2
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_bindir}/stt-daemon
/etc/dbus-1/session.d/stt-server.conf
%{TZ_SYS_RO_SHARE}/voice/stt/1.0/stt-config.xml
%{TZ_SYS_RO_SHARE}/dbus-1/services/org.tizen.voice*
%{TZ_SYS_RO_SHARE}/voice/test/stt-test
%{TZ_SYS_RO_SHARE}/license/%{name}

%files devel
%manifest %{name}-devel.manifest
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/stt.pc
%{_includedir}/stt.h

%files file-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/stt-file.pc
%{_includedir}/stt_file.h

%files setting-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/stt-setting.pc
%{_includedir}/stt_setting.h

%files engine-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/stt-engine.pc
%{_includedir}/sttp.h
