%bcond_without observation
%define DIRNAME grid-admin
%define SPECNAME smartmet-plugin-%{DIRNAME}
Summary: SmartMet grid admin plugin
Name: %{SPECNAME}
Version: 21.3.5
Release: 1%{?dist}.fmi
License: MIT
Group: SmartMet/Plugins
URL: https://github.com/fmidev/smartmet-plugin-grid-admin
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: boost169-devel
BuildRequires: libconfig-devel
BuildRequires: omniORB-devel
BuildRequires: smartmet-library-spine-devel >= 21.3.1
BuildRequires: smartmet-library-locus-devel >= 21.2.18
BuildRequires: smartmet-library-macgyver-devel >= 21.2.25
BuildRequires: smartmet-library-newbase-devel >= 21.3.2
BuildRequires: smartmet-library-grid-files-devel >= 21.3.4
BuildRequires: smartmet-library-grid-content-devel >= 21.3.4
BuildRequires: smartmet-engine-grid-devel >= 21.3.3
BuildRequires: gdal32-devel
Requires: libconfig
Requires: smartmet-library-locus >= 21.2.18
Requires: smartmet-library-macgyver >= 21.2.25
Requires: smartmet-library-newbase >= 21.3.2
Requires: smartmet-library-spine >= 21.3.1
Requires: smartmet-engine-grid >= 21.3.3
Requires: smartmet-server >= 21.1.14
#Requires: boost169-date-time
#Requires: boost169-filesystem
#Requires: boost169-iostreams
#Requires: boost169-regex
#Requires: boost169-system
#Requires: boost169-thread
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
* Fri Mar  5 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.3.5-1.fmi
- Detect if the grid-engine is disabled

* Wed Mar  3 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.3.3-1.fmi
- Repackaged due to grid-files API changes

* Wed Feb  3 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.3-1.fmi
- Repackaged due to base library API changes

* Wed Jan 27 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.27-1.fmi
- Repackaged due to base library ABI changes

* Tue Jan 19 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.19-1.fmi
- Repackaged due to base library ABI changes

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues

* Mon Jan 11 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.11-1.fmi
- Repackaged due to grid-files API changes

* Mon Jan  4 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.4-1.fmi
- Repackaged due to base library API changes

* Tue Dec 29 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.12.29-1.fmi
- GDAL upgrade

* Thu Dec  3 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.12.3-1.fmi
- Repackaged since RedisImplementation ABI changed

* Mon Nov 30 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.11.30-1.fmi
- Repackaged due to grid-content library API changes

* Tue Nov 24 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.11.24-1.fmi
- Moved grid-engine browsing into the grid engine module
- Improved data and configuration browsing

* Thu Oct 22 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.10.22-1.fmi
- Repackaged due to library ABI changes

* Thu Oct 15 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.10.15-1.fmi
- Repackaged due to library ABI changes

* Wed Oct  7 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.10.7-1.fmi
- Repackaged due to library ABI changes

* Thu Oct  1 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.10.1-1.fmi
- Repackaged due to library ABI changes

* Wed Sep 23 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.9.23-1.fmi
- Use Fmi::Exception instead of Spine::Exception

* Fri Sep 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.9.18-1.fmi
- Small updates to parameter handling

* Tue Sep 15 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.9.15-1.fmi
- Repackaged due to library ABI changes

* Mon Sep 14 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.9.14-1.fmi
- Repackaged due to library ABI changes

* Tue Sep  8 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.9.8-1.fmi
- Repackaged due to library ABI changes

* Mon Aug 31 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.31-1.fmi
- Repackaged due to library ABI changes

* Fri Aug 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.21-1.fmi
- Upgrade to fmt 6.2

* Fri Aug 14 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.14-1.fmi
- Repackaged due to grid library ABI changes

* Mon Jun  8 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.6.8-1.fmi
- Repackaged due to base library changes

* Fri May 15 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.5.15-1.fmi
- Repackaged due to base library changes

* Wed May  6 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.5.6-1.fmi
- Fixed content type to be text/plain for method calls

* Tue May  5 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.5.5-2.fmi
- Added content type header

* Tue May  5 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.5.5-1.fmi
- Added contentServer browsing functionality

* Thu Apr 30 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.30-1.fmi
- Repackaged due to base library API changes

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgraded to Boost 1.69

* Fri Apr  3 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.3-1.fmi
- Repackaged due to library API changes

* Thu Mar  5 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.3.5-1.fmi
- Repackaged due to library API changes

* Tue Feb 25 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.25-1.fmi
- Repackaged due to library API changes

* Wed Feb 19 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.19-1.fmi
- Repackaged due to library API changes

* Wed Jan 29 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.1.29-1.fmi
- Repackaged due to library API changes

* Tue Jan 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.1.21-1.fmi
- Repackaged due to grid-content and grid-engine API changes

* Thu Jan 16 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.1.16-1.fmi
- Repackaged due to base library API changes

* Wed Dec 11 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.12.11-1.fmi
- Repackaged due to small API changes in base libraries

* Wed Dec  4 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.12.4-1.fmi
- Repackaged due to base library changes

* Fri Nov 22 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.22-1.fmi
- Repackaged due to API changes in grid-content library

* Wed Nov 20 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.20-1.fmi
- Repackaged due to newbase/spine ABI changes

* Thu Nov  7 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.7-1.fmi
- Repackaged due to library/engine changes

* Fri Oct 25 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.25-1.fmi
- Full repackaging of the GRIB server components

* Tue Oct  1 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.1-1.fmi
- Repackaged due to SmartMet library ABI changes

* Thu Sep 19 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.19-1.fmi
- New release version

* Fri Aug  9 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.8.9-1.fmi
- Version update

* Mon May  6 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.5.6-1.fmi
- Updated dependencies

* Tue Mar 19 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.3.19-1.fmi
- Repackaged due to API changes

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
