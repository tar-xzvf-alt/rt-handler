# RT Handler — GPIO-монитор real-time задержек на одноплатниках

С-демон, работающий на стороне одноплатника: отвечает на GPIO-импульсы от
Arduino (тестера) встречным сигналом на выходной линии. Является частью
проекта [rt-tester](https://altlinux.space/besogon1238/rt-tester).

## Поддерживаемые платы

| Платформа | Архитектура | GPIO Chip | Consumer |
|-----------|-------------|-----------|----------|
| Lichee RV Dock | RISC-V | `/dev/gpiochip0` | `lichee-monitor` |
| Radxa RK3588 5B | ARM | `/dev/gpiochip3` | `radxa-monitor` |
| Starfive Visionfive 2 | RISC-V | `/dev/gpiochip0` | `starfive-monitor` |
| БЦВМ АРМ | ARM | `/dev/gpiochip0` | `bcvm-monitor` |
| БВЦ АРМ | ARM | `/dev/gpiochip0` | `bvcarm-mo` |
| MangoPi MQ Pro | RISC-V | `/dev/gpiochip0` | `mangopi-monitor` |
| RockPI 4 | ARM | `/dev/gpiochip4` | `rockpi4-monitor` |
| RepkaPI 4 | ARM | `/dev/gpiochip1` | `repkapi4-monitor` |

## Зависимости (ALT Linux)

```bash
apt-get install gcc make libgpiod-devel
```

## Сборка

```bash
cd src && make
```

Бинарь: `rt-handler` — универсальный, плата выбирается при запуске.

## Использование

```bash
# Выбор платы из встроенного пресета
rt-handler -b starfive

# Загрузка из конфиг-файла
rt-handler -c /etc/rt-handler/boards.d/starfive.conf

# Пресет + переопределение параметров из файла
rt-handler -b starfive -c /etc/rt-handler/boards.d/custom.conf

# Список доступных плат
rt-handler -h
```

### Формат конфиг-файла

```ini
GPIO_CHIP=/dev/gpiochip0
GPIO_OFFSET_IN=60
GPIO_OFFSET_OUT=61
GPIO_EDGE=both
GPIO_MODE_TOGGLE=0
GPIO_CONSUMER=starfive-monitor
```

Ключи: `GPIO_EDGE` — `both` / `rising` / `falling` / `none`, `GPIO_MODE_TOGGLE` — `0` или `1`.

Готовые конфиги для каждой платы находятся в `conf/boards.d/` и при установке RPM копируются в `/etc/rt-handler/boards.d/`.

## Конфигурация GPIO по платам

| Платформа | Вход (от Arduino pin 7) | Выход (к Arduino pin 20) |
|-----------|------------------------|--------------------------|
| Lichee RV Dock | offset 140 | offset 144 |
| Radxa RK3588 5B | offset 10 | offset 11 |
| Starfive Visionfive 2 | offset 60 | offset 61 |
| БЦВМ АРМ | offset 15 | offset 9 |
| БВЦ АРМ | offset 0 | offset 4 |
| MangoPi MQ Pro | offset 35 | offset 36 |
| RockPI 4 | offset 6 | offset 7 |
| RepkaPI 4 | offset 205 | offset 204 |

## Установка и запуск как systemd-сервис

```bash
# Сборка RPM (на сборочной машине или на самой плате)
tar xf rt-handler-0.2.0.tar.gz
cd rt-handler-0.2.0/src && make
rpmbuild -ba rt-handler.spec

# Установка
apt-get install rt-handler-*.riscv64.rpm

# Управление сервисом
systemctl enable --now rt-handler    # запустить и добавить в автозагрузку
systemctl status rt-handler          # статус
systemctl stop rt-handler            # остановить
systemctl start rt-handler           # запустить
journalctl -u rt-handler             # просмотр логов

# Смена платы
rt-handler-set-board lichee           # переключить на Lichee RV Dock
rt-handler-set-board starfive         # вернуть на VisionFive 2
```

## Конфигурация сервиса

Плата по умолчанию хранится в `/etc/sysconfig/rt-handler`:

```
BOARD=starfive
```

Сменить плату — один из двух способов:

1. **Команда** `rt-handler-set-board <board>` — меняет плату и сразу перезапускает сервис.
2. **Вручную** — отредактировать `/etc/sysconfig/rt-handler` и выполнить `systemctl restart rt-handler`.

## Архитектура

```
Arduino Mega ──(GPIO импульс)────────────────→ Одноплатник (rt-handler)
   pin 7  →                                      ├─ Обработчик GPIO: ждёт фронт/спад
   pin 20 ←                                      └─ Выставляет ответ на выходной линии
```

## Связанные проекты

- [rt-tester](https://altlinux.space/besogon1238/rt-tester) — ПК-приёмник, прошивка Arduino, Prometheus/Grafana, результаты тестирования
- [rt-supervisor](https://altlinux.space/besogon1238/rt-supervisor) — Ethernet-супервизор для сквозного тестирования
