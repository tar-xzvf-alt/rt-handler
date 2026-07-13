# RT Handler — С-код для тестирования real-time задержек на одноплатниках

В этом репозитории находится С-код, работающий на стороне одноплатника: ответ на
GPIO-импульсы от Arduino, сбор метрик CPU/памяти через `/proc/stat` и `/proc/meminfo`,
запись статистики групп в лог-файл. Является частью проекта [rt-tester](https://altlinux.space/besogon1238/rt-tester).

## Поддерживаемые платы

| Платформа | Архитектура | GPIO Chip | Consumer | Документация |
|-----------|-------------|-----------|----------|-------------|
| Lichee RV Dock | RISC-V | `/dev/gpiochip0` | `lichee-monitor` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/lichee_tests.md) |
| Radxa RK3588 5B | ARM | `/dev/gpiochip3` | `radxa-monitor` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/radxa_tests.md) |
| Starfive Visionfive 2 | RISC-V | `/dev/gpiochip0` | `starfive-monitor` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/visionfive_tests.md) |
| БЦВМ АРМ | ARM | `/dev/gpiochip0` | `bcvm-monitor` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/bcvmarm_tests.md) |
| БВЦ АРМ | ARM | `/dev/gpiochip0` | `bvcarm-mo` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/bvcarm_tests.md) |
| MangoPi MQ Pro | RISC-V | `/dev/gpiochip0` | `mangopi-monitor` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/mangopi_tests.md) |
| RockPI 4 | ARM | `/dev/gpiochip4` | `rockpi4-monitor` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/rockpi4_tests.md) |
| RepkaPI 4 | ARM | `/dev/gpiochip1` | `repkapi4-monitor` | [📄](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/repkapi4_tests.md) |

## Зависимости (ALT Linux)

```bash
apt-get install gcc make libgpiod-devel
```

## Сборка

```bash
make BOARD=lichee
```

Где `BOARD` — одно из: `bvc`, `bcvm`, `lichee`, `radxa`, `starfive`, `mangopi`, `rockpi4`, `repkapi4`, `bvc_arm`.

После сборки бинарник называется `$(BOARD)-monitor` (например, `lichee-monitor`).

## Приоритеты реального времени

Для минимизации задержек используются политика `SCHED_FIFO` и три уровня приоритетов:

```
IRQ-поток (прерывание GPIO):   99 (SCHED_FIFO)  — максимальный
Обработчик GPIO:               80 (SCHED_FIFO)  — отвечает на импульсы
Сборщик метрик CPU/памяти:      79 (SCHED_FIFO)  — на 1 ниже обработчика
```

## Конфигурация GPIO

Пины для каждой платы заданы в `src/gpio_config.h` через `-DBOARD_xxx`:

| Платформа | Вход (от Arduino pin 7) | Выход (к Arduino pin 20) |
|-----------|------------------------|--------------------------|
| Lichee RV Dock | offset 140 (bank 4×32+12) | offset 144 (bank 4×32+16) |
| Radxa RK3588 5B | offset 10 (bank 1×8+2) | offset 11 (bank 1×8+3) |
| Starfive Visionfive 2 | offset 60 | offset 61 |
| БЦВМ АРМ | offset 15 | offset 9 |
| БВЦ АРМ | offset 0 | offset 4 |
| MangoPi MQ Pro | offset 35 (bank 1×32+3) | offset 36 (bank 1×32+4) |
| RockPI 4 | offset 6 | offset 7 |
| RepkaPI 4 | offset 205 (bank 6×32+13) | offset 204 (bank 6×32+12) |

## Архитектура

```
Arduino Mega ──(GPIO импульс)──→ Одноплатник (этот код)
  pin 7 →                        ├─ Обработчик GPIO (prio 80): ждёт фронт
  pin 20 ←                        ├─ Выставляет ответ на выходной линии
                                  └─ Сборщик метрик (prio 79): /proc/stat + /proc/meminfo → лог

Приёмник на ПК (receiver.py) читает данные от Arduino через serial.
```

## Связанные проекты

- [rt-tester](https://altlinux.space/besogon1238/rt-tester) — ПК-приёмник, прошивка Arduino, Prometheus/Grafana, результаты тестирования
- [rt-supervisor](https://altlinux.space/besogon1238/rt-supervisor) — Ethernet-супервизор + controller-emu для сквозного тестирования

Результаты тестирования плат хранятся в [rt-tester/tests/](https://altlinux.space/besogon1238/rt-tester/src/branch/main/tests/).
Ссылки на документацию по каждой плате — в таблице выше.
