%bcond_without observation
%define DIRNAME grid-admin
%define SPECNAME smartmet-plugin-%{DIRNAME}
Summary: SmartMet grid admin plugin
Name: %{SPECNAME}
Version: 19.2.15
Release: 1%{?dist}.fmi
License: MIT
Group: SmartMet/Plugins
URL: https://github.com/fmidev/smartmet-plugin-grid-admin
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: boost-devel
BuildRequires: libconfig-devel
BuildRequires: smartmet-library-spine-devel >= 18.12.13
BuildRequires: smartmet-library-locus-devel >= 18.11.16
BuildRequires: smartmet-library-macgyver-devel >= 18.11.24
BuildRequires: smartmet-library-newbase-devel >= 19.2.14
BuildRequires: smartmet-library-grid-files-devel >= 19.2.15
BuildRequires: smartmet-library-grid-content-devel >= 19.2.15
BuildRequires: smartmet-engine-grid-devel >= 19.2.15
BuildRequires: gdal-devel
Requires: libconfig
Requires: smartmet-library-locus >= 18.11.16
Requires: smartmet-library-macgyver >= 18.11.24
Requires: smartmet-library-newbase >= 19.2.14
Requires: smartmet-library-spine >= 18.12.13
Requires: smartmet-engine-grid >= 19.2.15
Requires: smartmet-server >= 18.12.14
#Requires: boost-date-time
#Requires: boost-filesystem
#Requires: boost-iostreams
#Requires: boost-regex
#Requires: boost-system
#Requires: boost-thread
Provides: %{SPECNAME}

%description
SmartMet grid admin plugin

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}

%build -q -n %{SPECNAME}
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_datadir}/smartmet/plugins/%{DIRNAME}.so

%changelog
* Fri Feb 15 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.2.15-1.fmi
- Version update

* Thu Jan 17 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.1.17-1.fmi
- Version update

* Wed Oct 24 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.10.24-1.fmi
- Downgraded spine dependency due to a bad rpm version number

* Mon Oct 15 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.10.15-1.fmi
- Added module description

* Wed Sep 26 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.26-1.fmi
- Version update

* Mon Sep 10 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.10-1.fmi
- Version update

* Thu Aug 30 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.30-1.fmi
- Silenced CodeChecker warnings

* Wed Aug 29 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.29-1.fmi
- Update to latest version

* Thu Jun 14 2018 Roope Tervo <roope.tervo@fmi.fi> - 18.6.14-1.fmi
- initial build
