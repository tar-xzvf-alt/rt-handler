# RT Handler — GPIO monitor for real-time latency testing on SBCs

C daemon running on a single-board computer: responds to GPIO impulses from
an Arduino (tester) with an output line signal. Part of the
[rt-tester](https://altlinux.space/besogon1238/rt-tester) project.

## Supported boards

| Platform | Architecture | GPIO Chip | Consumer |
|----------|-------------|-----------|----------|
| Lichee RV Dock | RISC-V | `/dev/gpiochip0` | `lichee-monitor` |
| Radxa RK3588 5B | ARM | `/dev/gpiochip3` | `radxa-monitor` |
| Starfive Visionfive 2 | RISC-V | `/dev/gpiochip0` | `starfive-monitor` |
| BCVM ARM | ARM | `/dev/gpiochip0` | `bcvm-monitor` |
| BVC ARM | ARM | `/dev/gpiochip0` | `bvcarm-mo` |
| MangoPi MQ Pro | RISC-V | `/dev/gpiochip0` | `mangopi-monitor` |
| RockPI 4 | ARM | `/dev/gpiochip4` | `rockpi4-monitor` |
| RepkaPI 4 | ARM | `/dev/gpiochip1` | `repkapi4-monitor` |

## Dependencies (ALT Linux)

```bash
apt-get install gcc make libgpiod-devel
```

## Build

```bash
cd src && make
```

Produces a single binary `rt-handler` — board is selected at runtime.

### Cross-build RPM with gear-hsh (ALT Linux)

```bash
gear-hsh --no-sisyphus-check -v \
  --apt-config=/path/to/apt/riscv64.conf \
  --target riscv64 \
  --nprocs=12 \
  /path/to/hasher/riscv64_chroot/
```

## Usage

```bash
# Select board from built-in preset
rt-handler -b starfive

# Load from config file
rt-handler -c /etc/rt-handler/boards.d/starfive.conf

# Preset + overrides from config file
rt-handler -b starfive -c /etc/rt-handler/boards.d/custom.conf

# List available boards
rt-handler -h
```

### Config file format

```ini
GPIO_CHIP=/dev/gpiochip0
GPIO_OFFSET_IN=60
GPIO_OFFSET_OUT=61
GPIO_EDGE=both
GPIO_MODE_TOGGLE=0
GPIO_CONSUMER=starfive-monitor
```

Keys: `GPIO_EDGE` — `both` / `rising` / `falling` / `none`, `GPIO_MODE_TOGGLE` — `0` or `1`.

Ready-to-use configs for each board are in `conf/boards.d/`. RPM installation
copies them to `/etc/rt-handler/boards.d/`.

## GPIO pinout by board

| Platform | Input (from Arduino pin 7) | Output (to Arduino pin 20) |
|----------|---------------------------|---------------------------|
| Lichee RV Dock | offset 140 | offset 144 |
| Radxa RK3588 5B | offset 10 | offset 11 |
| Starfive Visionfive 2 | offset 60 | offset 61 |
| BCVM ARM | offset 15 | offset 9 |
| BVC ARM | offset 0 | offset 4 |
| MangoPi MQ Pro | offset 35 | offset 36 |
| RockPI 4 | offset 6 | offset 7 |
| RepkaPI 4 | offset 205 | offset 204 |

## Installation and systemd service

```bash
# Build RPM
gear-hsh --no-sisyphus-check -v \
  --apt-config=/path/to/apt/riscv64.conf \
  --target riscv64 \
  --nprocs=12 \
  /path/to/hasher/riscv64_chroot/

# Install
apt-get install rt-handler-*.riscv64.rpm

# Manage service
systemctl enable --now rt-handler    # start and enable at boot
systemctl status rt-handler          # status
systemctl stop rt-handler            # stop
systemctl start rt-handler           # start
journalctl -u rt-handler             # view logs
```

## Service configuration

Default board is stored in `/etc/sysconfig/rt-handler`:

```
BOARD=starfive
```

Two ways to switch:

1. **Command** `rt-handler-set-board <board>` — changes the board and restarts
   the service immediately.
2. **Manually** — edit `/etc/sysconfig/rt-handler` then
   `systemctl restart rt-handler`.

### rt-handler-set-board

```bash
rt-handler-set-board starfive      # switch to VisionFive 2
rt-handler-set-board lichee        # switch to Lichee RV Dock
rt-handler-set-board list          # list available boards
rt-handler-set-board show          # show current board
rt-handler-set-board --help        # show usage
```

### Troubleshooting: stale unit override

If `rt-handler-set-board` does not change the running board, a leftover unit
file in `/etc/systemd/system/` may shadow the packaged unit:

```bash
systemctl cat rt-handler           # check which file is loaded
systemctl revert rt-handler        # remove local override
systemctl daemon-reload
systemctl restart rt-handler
```

## Architecture

```
Arduino Mega ──(GPIO impulse)─────────────→ SBC (rt-handler)
   pin 7  →                                  ├─ GPIO handler: waits for edge
   pin 20 ←                                  └─ Toggles output line in response
```

## Related projects

- [rt-tester](https://altlinux.space/besogon1238/rt-tester) — PC receiver,
  Arduino firmware, Prometheus/Grafana, test results
- [rt-supervisor](https://altlinux.space/besogon1238/rt-supervisor) — Ethernet
  supervisor for end-to-end testing

## License

[GPLv3](LICENSE.md)
