# RT Handler - GPIO monitor for real-time latency testing on SBCs

C daemon running on a single-board computer: responds to GPIO impulses from
an Arduino (tester) with an output line signal. Part of the
[rt-tester](https://altlinux.space/besogon1238/rt-tester) project.

RT Handler has one standalone role: it handles the GPIO request/response path
on the SBC. It does not open a network interface, send or receive raw Ethernet
frames, or provide the raw-Ethernet supervisor/receiver stack. Those parts are
separate programs in the related projects.

## Supported board presets

The `-b` option accepts exactly these nine compiled preset names:

| CLI name | Board / human name | Architecture | GPIO chip | Input | Output | Edge / behavior |
|----------|--------------------|--------------|-----------|-------|--------|-----------------|
| `bcvm` | BCVM ARM | ARM | `/dev/gpiochip0` | 15 | 9 | falling / toggle |
| `bvc` | BVC ARM | ARM | `/dev/gpiochip0` | 0 | 4 | both / mirror |
| `bvc_arm` | BVC ARM (alias of `bvc`) | ARM | `/dev/gpiochip0` | 0 | 4 | both / mirror |
| `lichee` | Lichee RV Dock | RISC-V | `/dev/gpiochip0` | 140 | 144 | both / mirror |
| `radxa` | Radxa RK3588 5B | ARM | `/dev/gpiochip3` | 10 | 11 | both / mirror |
| `starfive` | StarFive VisionFive 2 | RISC-V | `/dev/gpiochip0` | 60 | 61 | both / mirror |
| `mangopi` | MangoPi MQ Pro | RISC-V | `/dev/gpiochip0` | 35 | 36 | both / mirror |
| `rockpi4` | Rock Pi 4 | ARM | `/dev/gpiochip4` | 6 | 7 | both / mirror |
| `repkapi4` | Repka Pi 4 | ARM | `/dev/gpiochip1` | 205 | 204 | both / mirror |

In mirror mode (`GPIO_MODE_TOGGLE=0`), a rising input event sets the output
high and a falling event sets it low. In toggle mode
(`GPIO_MODE_TOGGLE=1`), each detected event writes the next alternating output
value; the sequence starts by writing low, then high. The `bcvm` preset uses
toggle mode because it watches falling edges only. All other compiled presets
use both edges and mirror mode.

## Dependencies (ALT Linux)

```bash
apt-get install gcc make libgpiod-devel
```

## Build

```bash
cd src
make
```

This produces the single `src/rt-handler` binary. The board is selected at
runtime.

## Usage

```bash
# Select a compiled board preset
rt-handler -b starfive

# Use a complete custom configuration without a preset
rt-handler -c ./custom.conf

# Apply only the keys present in the file after loading the preset
rt-handler -b starfive -c ./custom.conf

# Print usage and the exact compiled preset list
rt-handler -h
```

At least one of `-b` or `-c` is required. If both are given, the compiled `-b`
preset is loaded first and each recognized key present in the `-c` file
overrides that value. A file used without `-b` must provide `GPIO_CHIP`,
`GPIO_OFFSET_IN`, `GPIO_OFFSET_OUT`, `GPIO_EDGE`, and `GPIO_CONSUMER`;
`GPIO_MODE_TOGGLE` defaults to false when omitted.

The `-c` argument is always an explicit file path. The program does not search
`/etc/rt-handler/boards.d/`, derive a file name from `-b`, or automatically
merge an installed board file.

### Config file format

```ini
GPIO_CHIP=/dev/gpiochip0
GPIO_OFFSET_IN=60
GPIO_OFFSET_OUT=61
GPIO_EDGE=both
GPIO_MODE_TOGGLE=0
GPIO_CONSUMER=starfive-monitor
```

`GPIO_EDGE` accepts `both`, `rising`, `falling`, or `none`.
`GPIO_MODE_TOGGLE` accepts `0`, `1`, `false`, `true`, `no`, or `yes`.

The source tree contains matching examples in `conf/boards.d/`; the RPM
installs them in `/etc/rt-handler/boards.d/`. They are configuration examples
for explicit `-c` use and provide the file-name allowlist used by
`rt-handler-set-board`. They are not automatically loaded by the service.

## RPM build and installation

Run `gear-hsh` as a regular user configured for hasher (normally a member of
the `hashman` group). Use an architecture-specific apt configuration and a
separate hasher work directory for each target.

### riscv64

```bash
gear-hsh --no-sisyphus-check -v \
  --apt-config=/path/to/apt/riscv64.conf \
  --target riscv64 \
  --nprocs=12 \
  /path/to/hasher/riscv64

sudo apt-get install \
  /path/to/hasher/riscv64/repo/riscv64/RPMS.hasher/rt-handler-0.1.6-alt1.riscv64.rpm
```

### aarch64

```bash
gear-hsh --no-sisyphus-check -v \
  --apt-config=/path/to/apt/aarch64.conf \
  --target aarch64 \
  --nprocs=12 \
  /path/to/hasher/aarch64

sudo apt-get install \
  /path/to/hasher/aarch64/repo/aarch64/RPMS.hasher/rt-handler-0.1.6-alt1.aarch64.rpm
```

Install the resulting RPM on a machine of the matching architecture. Package
installation requires root privileges; the hasher build itself does not.

### RPM contents

Version `0.1.6-alt1` contains these payload files:

```text
/usr/sbin/rt-handler
/usr/sbin/rt-handler-set-board
/etc/rt-handler/boards.d/bcvm.conf
/etc/rt-handler/boards.d/bvc.conf
/etc/rt-handler/boards.d/bvc_arm.conf
/etc/rt-handler/boards.d/lichee.conf
/etc/rt-handler/boards.d/mangopi.conf
/etc/rt-handler/boards.d/radxa.conf
/etc/rt-handler/boards.d/repkapi4.conf
/etc/rt-handler/boards.d/rockpi4.conf
/etc/rt-handler/boards.d/starfive.conf
/etc/sysconfig/rt-handler
/lib/systemd/system/rt-handler.service
/usr/share/doc/rt-handler-0.1.6/README.md
/usr/share/doc/rt-handler-0.1.6/LICENSE.md
```

The files under `/etc/rt-handler/boards.d/` and
`/etc/sysconfig/rt-handler` are `%config(noreplace)` configuration. The
package's systemd scriptlets register the unit on initial installation and
remove it on package removal according to ALT systemd policy. Use
`systemctl enable --now` explicitly when the service must start now and at
boot.

### Package verification

```bash
# Verify the RPM digest/signature and inspect its payload and scriptlets
rpm -K /path/to/rt-handler-0.1.6-alt1.riscv64.rpm
rpm -qpl /path/to/rt-handler-0.1.6-alt1.riscv64.rpm
rpm -qp --scripts /path/to/rt-handler-0.1.6-alt1.riscv64.rpm

# Verify installed package files; no output means no differences
rpm -V rt-handler

# Confirm the effective unit and process status
systemctl cat rt-handler
systemctl status rt-handler
```

Replace the RPM suffix with `.aarch64.rpm` for the aarch64 build.

## systemd service

The packaged `/etc/sysconfig/rt-handler` defaults to:

```text
BOARD=starfive
```

The unit reads that optional environment file and starts exactly:

```text
/usr/sbin/rt-handler -b ${BOARD}
```

Consequently, `BOARD` must be one of the nine compiled preset names. The unit
does not pass `-c`, so editing a file in `/etc/rt-handler/boards.d/` does not
change the running configuration. To run overrides from a file, invoke the
binary directly or create an intentional local unit override that adds `-c`.

```bash
sudo systemctl enable --now rt-handler
systemctl status rt-handler
sudo systemctl stop rt-handler
sudo systemctl start rt-handler
journalctl -u rt-handler
```

The service runs as root because the unit has no `User=` setting and needs
access to GPIO character devices. It restarts after failures with a five-second
delay, but not after a clean exit or an explicit stop. The unit allows GPIO
character-device access, makes the system filesystem read-only except for
`/dev`, `/sys`, and `/proc`, uses a private temporary directory, prevents new
privileges, and sets `LimitRTPRIO=99`. It is ordered after `network.target`
(although the daemon itself does no networking) and enabling it adds it to
`multi-user.target`.

For manual execution, use root or an account with sufficient permissions on
the selected `/dev/gpiochipN` device. Changing the packaged system setting and
restarting the service also requires administrative privileges.

### rt-handler-set-board

```bash
sudo rt-handler-set-board starfive  # write BOARD=starfive and restart service
sudo rt-handler-set-board lichee    # write BOARD=lichee and restart service
rt-handler-set-board list           # list installed boards.d file names
rt-handler-set-board show           # show BOARD from /etc/sysconfig/rt-handler
rt-handler-set-board --help         # show usage
```

For a board argument, the helper first requires a matching
`/etc/rt-handler/boards.d/<board>.conf`, then replaces
`/etc/sysconfig/rt-handler` with `BOARD=<board>` at mode `0644`, and restarts
the service. This validates the name through the packaged example file; it
does not cause that file's GPIO values to be loaded. Run the board-changing
form as root because it writes under `/etc` and calls `systemctl restart`.

### Troubleshooting: stale unit override

If `rt-handler-set-board` does not change the running board, a leftover unit
file in `/etc/systemd/system/` may shadow the packaged unit:

```bash
systemctl cat rt-handler
sudo systemctl revert rt-handler
sudo systemctl daemon-reload
sudo systemctl restart rt-handler
```

## Architecture

```text
Arduino Mega --(GPIO impulse)-----------> SBC (rt-handler)
   pin 7  ->                                GPIO input: waits for edge
   pin 20 <-                                GPIO output: mirrors or toggles
```

No Ethernet packet handling occurs in this daemon.

## Related projects

- [rt-tester](https://altlinux.space/besogon1238/rt-tester) - PC receiver,
  Arduino firmware, Prometheus/Grafana, test results
- [rt-supervisor](https://altlinux.space/besogon1238/rt-supervisor) - Ethernet
  supervisor for end-to-end testing

## License

[GPLv3](LICENSE.md)
