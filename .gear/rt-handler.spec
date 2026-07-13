Name: rt-handler
Version: 0.1.2
Release: alt1
Summary: Real-time GPIO monitor for SBC latency testing
License: GPLv3
Group: System/Servers
Url: https://altlinux.space/besogon1238/rt-handler

BuildRequires: gcc make libgpiod-devel
Requires: libgpiod2

Source0: %name-%version.tar

%description
RT Handler responds to GPIO impulses and toggles an output line
in response. Used for real-time latency benchmarking on ARM
and RISC-V single-board computers (part of rt-tester project).

%prep
%setup

%build
cd src
%make_build

%install
install -Dm755 src/%name %buildroot%_sbindir/%name
install -Dm755 deploy/rt-handler-set-board %buildroot%_sbindir/rt-handler-set-board
install -d %buildroot%_sysconfdir/%name/boards.d
install -m644 conf/boards.d/*.conf %buildroot%_sysconfdir/%name/boards.d/
install -d %buildroot%_sysconfdir/sysconfig
echo 'BOARD=starfive' > %buildroot%_sysconfdir/sysconfig/%name
install -Dm644 deploy/%name.service %buildroot%_unitdir/%name.service

%post
%systemd_post %name.service

%preun
%systemd_preun %name.service

%files
%_sbindir/rt-handler
%_sbindir/rt-handler-set-board
%config(noreplace) %_sysconfdir/rt-handler/boards.d/
%config(noreplace) %_sysconfdir/sysconfig/rt-handler
%_unitdir/%name.service

%changelog
* Mon Jul 13 2026 Taran Evgeniy <taranev@basealt.ru> 0.1.2-alt1
- Translate README, switch license to GPLv3 

* Mon Jul 13 2026 Taran Evgeniy <taranev@basealt.ru> 0.1.1-alt1
- Add board selection via /etc/sysconfig, rt-handler-set-board utility

* Mon Jul 13 2026 Taran Evgeniy <taranev@basealt.ru> 0.1.0-alt2
- Update packaging

* Mon Jul 13 2026 Taran Evgeniy <taranev@basealt.ru> 0.1.0-alt1
- Initial version
