%define name    db2tcl
%define version 0.0.2

Summary: TCL extension for IBM DB2
Name: %{name}
Version: %{version}
Release: 1
Copyright: BSD
Group: Applications/Databases
Source: http://prdownloads.sourceforge.net/db2tcl/%{name}-%{version}.tar.gz
URL: http://db2tcl.sourceforge.net/
Packager: Sergey N. Belinsky <sergeybe@users.sourceforge.net>
BuildRoot: %{_tmppath}/%{name}-buildroot/
BuildRequires: tcl >= 8.0.0  tk >= 8.0.0

%description
DB2TCL is an extension to the TCL that provides access 
to a IBM UDB DB2 database server.

%description -l ru
DB2TCL это расширение для языка програмирования TCL, которое 
позволяет получит доступ к серверу баз данных IBM UDB DB2.

%prep

%setup

%configure

%build

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%clean
rm -rf %{buildroot}


%files
%defattr(-, root, root)
%doc README COPYING INSTALL TODO ChangeLog 
/usr/bin/*
/usr/lib/*
/usr/man/man1/*

%changelog
* Thu Dec  5 2002 Sergey N. Belinsky <sergeybe@users.sourceforge.net>
- Initial spec
