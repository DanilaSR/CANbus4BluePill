# Термостат

Это версия официального демо PyCyphal на C++

## Виртуальная CAN-шина в Linux

- Создаём:
  ```bash
  sudo ip link add vcan0 type vcan
  sudo ip link set vcan0 up
  ```
- Проверяем, что работает:
  ```bash
  ip link show vcan0
  ```

## Сборка проекта

Из корня проекта

```bash
cmake -S . -B build
cmake --build build
```

## Запуск

Для запуска выполняем

```bash
./build/thermostat
```

Дальше можно потыкать это дело с помощью утилиты
[yakut](https://github.com/opencyphal/yakut).
Для корректной работы `yakut` должно быть настроено окружение,
а именно следующие переменные:

```bash
export CYPHAL_PATH="path/to/cyphal_types/public_regulated_data_types"
export UAVCAN__CAN__IFACE="socketcan:vcan0"
```

- посмотреть `hearbeat`:

```bash
y sub --with-metadata uavcan.node.heartbeat
```

- в одном терминале мониторить напряжение,
  в другом - отправлять температуру

```bash
y sub 2347:uavcan.si.unit.voltage.scalar --redraw
```

```bash
y pub --count=10 2345:uavcan.si.unit.temperature.scalar   250 \
                 2346:uavcan.si.sample.temperature.scalar 'kelvin: 240'
```
