Name:       stt
Summary:    Speech To Text client library and daemon
Version:    0.2.53
Release:    1
Group:      Graphics & UI Framework/Libraries
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


cmake . -DCMAKE_INSTALL_PREFIX=/usr -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
install LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}

%make_install

%post 
/sbin/ldconfig

mkdir -p /usr/lib/voice

mkdir -p /usr/share/voice

mkdir -p /opt/home/app/.voice
chown 5000:5000 /opt/home/app/.voice

mkdir -p /opt/usr/data/voice/stt/1.0
chown 5000:5000 /opt/usr/data/voice
chown 5000:5000 /opt/usr/data/voice/stt
chown 5000:5000 /opt/usr/data/voice/stt/1.0


%postun -p /sbin/ldconfig

/usr/bin/vconftool set -t string db/voice_input/language "auto" -g 5000 -f -s system::vconf_inhouse

%files
%manifest %{name}.manifest
%license LICENSE.APLv2
%defattr(-,root,root,-)
%{_libdir}/libstt.so
%{_libdir}/libstt_file.so
%{_libdir}/libstt_setting.so
/usr/lib/voice/stt/1.0/stt-config.xml
%{_bindir}/stt-daemon
/opt/usr/devel/bin/stt-test
/usr/share/license/%{name}

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
