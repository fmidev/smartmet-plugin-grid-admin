%bcond_without observation
%define DIRNAME grid-admin
%define SPECNAME smartmet-plugin-%{DIRNAME}
Summary: SmartMet grid admin plugin
Name: %{SPECNAME}
Version: 18.8.29
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
BuildRequires: smartmet-library-spine-devel >= 18.8.20
BuildRequires: smartmet-library-locus-devel >= 18.8.21
BuildRequires: smartmet-library-macgyver-devel >= 18.8.20
BuildRequires: smartmet-library-newbase-devel >= 18.8.28
BuildRequires: smartmet-library-grid-files-devel >= 18.8.28
BuildRequires: smartmet-library-grid-content-devel >= 18.8.28
BuildRequires: smartmet-engine-grid-devel >= 18.8.27
BuildRequires: gdal-devel
Requires: libconfig
Requires: smartmet-library-locus >= 18.8.21
Requires: smartmet-library-macgyver >= 18.8.20
Requires: smartmet-library-newbase >= 18.8.28
Requires: smartmet-library-spine >= 18.8.20
Requires: smartmet-engine-grid >= 18.8.27
Requires: smartmet-server >= 18.8.22
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
* Wed Aug 29 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.29-1.fmi
- Update to latest version

* Thu Jun 14 2018 Roope Tervo <roope.tervo@fmi.fi> - 18.6.14-1.fmi
- initial build
