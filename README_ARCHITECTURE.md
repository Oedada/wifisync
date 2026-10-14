# Wifisync — Архитектура проекта

## 📋 Обзор

**Wifisync** — децентрализованная файловая синхронизация между устройствами в локальной сети через P2P соединение. Проект имеет 4 основных компонента:

| Компонент | Описание | Статус |
|-----------|----------|--------|
| **C++ Core** | Sync-движок (LAN discovery, diff, transport, crypto) | ⚠️ В незавершенном рефакторинге |
| **Python GUI** | PyQt6 десктопное приложение | ❌ Удален на `test-dart-websockets` |
| **Dart CLI** | Командная строка | 🚧 WIP |
| **Flutter App** | Мобильное/десктоп приложение | 🚧 WIP |

---

## 🏗️ Структура модулей

```
include/           // Хэддеры
├── core/          // Синхронизация и диф-алгоритмы
├── fs/            // Файловый ввод/вывод, пути, persistence
├── crypto/        // Криптография (X25519, Ed25519, Hash)
├── network/       // Сетевой слой (TCP, UDP, HTTP)
├── util/          // Утилиты и константы

src/               // Реализации
│── main.cpp       // Старая точка входа (закомментирована)
│── threads.cpp    // Оркестрация sync (
│── *.cpp          // Все синхронизации модулей
││   ├── core/*    // Реализации core
│   ├── fs/*       // Реализации fs
│   ├── crypto/*   // Реализации crypto
│   ├── network/*  // Реализации network
│   └── util/*     // Реализации util

```

---

## 📦 Подробное описание классов

### 🎯 Core Module — Синхронизация и диф-алгоритмы

#### `Difference` (include/core/difference.hpp)
- **Назначение**: Вычисление разницы между двумя деревьями файлов
- **Поля**:
  - `ltree` — previous snapshot (JSON)
  - `ctree` — current snapshot (JSON)
  - `name` — имя файла
  - `dif` — результат diff (JSON)
- **Методы**:
  - `calculate_and_write_difference()` — расчет разницы и запись в файл
- **Связи**: `src/core/difference.cpp` использует `constants.hpp`, `filesystem`, `fstream`

#### `DifWork` (include/core/dif_work.hpp)
- **Назначение**: Планирование и управление циклом синхронизации
- **Поля**:
  - `other_uuid` — UUID remote-устройства
  - `own_uuid` — собственный UUID
  - `own_name` — собственное устройство
  - `dm` — `DataManager` (пersistence)
  - `detecting_paths` — пути для отслеживания
  - `ignoring_paths` — игнорируемые пути
- **Методы**:
  - `shift_snapshots()` — обновление снимков (⚠️ **BIT**:
  - `calculate_dif()` — расчет дифференциалов
  - `generate_dif_files()` — создание файлов diff
  - `get_next_diff_path()` — получение следующего diff-пути
  - `get_dif_work_path()` — получение пути к файлу работы
  - `diff()` — общий diff
- **Связи**: использует `FileAccess::` (смесь старого и нового!), `DataManager` (незав. migration)

#### `DifWalker` (include/core/dif_walker.hpp)
- **Назначение**: Обход diff-дерева
- **Методы** (предполагаются):
  - Обход по дереву diff
  - Извлечение изменений (create/delete)

#### `Transport` (include/core/transport.hpp)
- **Назначение**: P2P протокол передачи данных
- **Методы**:
  - `walk()` — основной процесс sync
  - `send_unit()` — отправка единицы данных
  - `receive_unit()` — получение единицы
  - Конфликты определяются в `Transport::walk_unit`
- **Структуры**:
  - `SUnit` — Send Unit (отправляется)
  - `RUnit` — Receive Unit (принимаем)

#### `ChangeApplier` (include/core/change_applier.hpp)
- **Назначение**: Применение изменений из diff
- **Методы**:
  - `apply()` — создание/удаление по diff

---

### 🗂️ FS Module — Файловый ввод/вывод

#### `FileAccess` (include/fs/environment.hpp)
- **Назначение**: Низкоуровневый I/O (открытие/закрытие файлов)
- **Методы**:
  - `static get_fin(string name)` — открыть для чтения
  - `static get_fout(string name)` — открыть для записи

#### `DataManager` (include/fs/data_manager.hpp)
- **Назначение**: Управление конфигурацией и устройствами
- **Структуры**:
  - `Paths` — пути к файлам (cfg, devices, snapshots)
  - `Config` — конфигурация устройства
  - `Device` — данные устройства
- **Методы**:
  - `get_name() const` — текущее имя
  - `get_uuid() const` — UUID устройства
  - `list_watch()` — список отслеживаемых путей
  - `list_ignore()` — список игноримых путей
  - Сохранение CFG/Device persistence
- **Связи**: `src/fs/data_manager.cpp` (реализация)

---

### 🔐 Crypto Module — Криптография

#### `X25519` (include/crypto/x25519.hpp)
- **Назначение**: Обмен ключами (Key Exchange)
- **Методы**:
  - `get_private_key()` — генерация закрытого ключа
  - `get_public_key()` — извлечение публичного ключа
  - `generate_bytes()` — генерация key bytes

#### `Ed25519` (include/crypto/ed25519.hpp)
- **Назначение**: Цифровые подписи
- **Методы**:
  - Подписывание данных
  - Проверка подписей

#### `Hash` (include/crypto/hash.hpp)
- **Назначение**: Хэшинг данных
- **Методы**:
  - `hash()` — вычисление хэша

---

### 🌐 Network Module — Сетевой слой

#### `UdpBroadcast` (include/network/broadcast.hpp)
- **Назначение**: UDP-бロードкаст для LAN discovery (порт 12312)
- **Структуры**:
  - `DeviceData` — данные найденного устройства (addr, name, last_seen)
  - `Message` — типы сообщений (Broadcast, ConnectRequest, ConnectResponse)
  - `State` — состояния (Discovering, Syncing)
- **Методы**:
  - `send()` — отправка сообщения
  - `recv()` — получение сообщения
  - `get_found_devices()` — список найденных устройств

#### `TCPSocket` (include/network/TCPSocket.hpp)
- **Назначение**: TCP соединение для P2P sync (порт 12345)
- **Методы**:
  - `send(string/vector<char>)` — отправка данных
  - `ip()` — получение IP
- **Особенности**: move-semantic (unique ownership)

#### `HTTPServer` (include/network/http_server.hpp)
- **Назначение**: HTTP REST API (порт 5000)
- **Связи**: использует `httplib` (FetchContent)
- **Методы**:
  - `start_server(int port)` — старт на порту
  - `add_handler()` — регистрация обработчиков
  - (Внимание: **НЕ ЗАПУСКАЕТСЯ** — `HTTPServer::start_server` не вызывается)
- **Функции**:
  - `json_ok()` — успешный ответ
  - `json_error()` — ошибочный ответ

#### `Server` (include/network/server.hpp)
- **Назначение**: TCP сервер для принятия соединений
- **Методы**:
  - `accept_conn()` — принятие соединения
  - `is_ready_to_accept()` — проверка готовностью
- **Поля**:
  - `server_port`, `server_fd`, `client_fd`

---

### 🛠️ Util Module — Утилиты

#### `Utils` (утилиты)
- **Функции** (предполагаются):
  - `logmsg()` — логирование
  - Вспомогательные функции

#### `Constants` (include/util/constants.hpp)
- **Константы**:
  - Ports: 12312, 12345, 5000
  - LAN IP адрес
  - Protocol strings
  - File paths

---

### 🧵 Sync Orchestration

#### `full_sync()` (src/threads.cpp)
- **Назначение**: Оркестрация полного цикла синхронизации
- **Методы**:
  - `full_sync()` — главный цикл sync
- **Проблема**: `main()` пустой, код закомментирован

#### `main()` (src/main.cpp)
- **Проблема**: Старая функция `main()` закомментирована
- **Статус**: неактивна

---

## 🔗 Взаимосвязи модулей

```
┌─────────────────────────────────────────────────────────────────────┐
│                         SYNC FLOW                                    │
└─────────────────────────────────────────────────────────────────────┘
              
┌─────────────┐      Broadcast (UDP 12312)             ┌──────────────┐
│ UdpBroadcast│ ──────────────────────────────────────>│ Server       │
│ (12312)     │                                         │ (accept)    │
└─────────────┘                                         └────────┬────┘
     │                                                        │
     │                                                        │
     │                                                        │
     ▼                                                        ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    HANDSHAKE (Crypto Layer)                         │
│  ┌──────────────┐    X25519 key exchange   ┌──────────────┐        │
│  │ X25519       │──────────────────────────>│ Ed25519      │        │
│  │  Key Exchange│                           │  Signatures   │        │
│  └──────────────┘                           └──────────────┘        │
│                                                                       │
│  ┌──────────────┐    Ed25519 auth    ┌──────────────┐               │
│  │ Hash          │──────────────────> │  Auth        │               │
│  └──────────────┘                      └──────────────┘               │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                    DATA TRANSFER (TCP 12345)                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌─────────────────┐                  ┌────────────────────┐      │
│   │ TCPSocket       │                  │ Transpor           │      │
│   │ (12345)         │──────────────────>│ ::walk()           │      │
│   └─────────────────┘                  └─────────┬───────────┘      │
│                                                 │                    │
│                                                 │                    │
│                                                 ▼                    │
│   ┌─────────────────┐                  ┌────────────────────┐      │
│   │ SUnit/RUnit      │                  │ ChangeApplier      │      │
│   │ (Data Packets)   │                  │ ::apply            │      │
│   └─────────────────┘                  └────────────────────┘      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                    DIFF GENERATION                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌──────────────────────────────────────────────────────┐         │
│   │ DifWork                                               │         │
│   │  ┌─────────────────────────────────────────────┐     │         │
│   │  │ shift_snapshots()                           │     │         │
│   │  │  - old: FileAccess::get_fin() (BROKEN!)     │     │         │
│   │  │  - cur: env::get_data_path() - OK           │     │         │
│   │  └─────────────────────────────────────────────┘     │         │
│   │                                                       │         │
│   │  ┌─────────────────────────────────────────────┐     │         │
│   │  │ calculate_dif()                             │     │         │
│   │  │  - Difference::calculate_and_write_diff()   │     │         │
│   │  └─────────────────────────────────────────────┘     │         │
│   │                                                       │         │
│   │  ┌─────────────────────────────────────────────┐     │         │
│   │  │ generate_dif_files()                        │     │         │
│   │  └─────────────────────────────────────────────┘     │         │
│   │                                                       │         │
│   │  ┌─────────────────────────────────────────────┐     │         │
│   │  │ Transport::walk()                           │     │         │
│   │  └─────────────────────────────────────────────┘     │         │
│   │                                                       │         │
│   │  ┌─────────────────────────────────────────────┐     │         │
│   │  │ ChangeApplier::apply()                      │     │         │
│   │  └─────────────────────────────────────────────┘     │         │
│   └──────────────────────────────────────────────────────┘         │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## ⚠️ Критические проблемы и незавершенные части

### 🔴 Высокий приоритет

#### 1. **`DifWork::shift_snapshots()` — битая строка** (строка 20)
```cpp
fs::path last_shapshot_path = FileAccess::;
//                                  ^--- ОШИБКА! Запятая отсутствует!
```

**Итог**: `build` невозможен, синхронизация сломана

**Решение**:
```cpp
fs::path last_shapshot_path = FileAccess::get_fin(last_snapshot_path);
```

#### 2. **HTTP-сервер никогда не запускается**
- `HTTPServer::start_server()` реализована, но **никогда не вызывается**
- GUI (`gui/gui/main.py`) пытается polling на `localhost:5000`
- **Итог**: GUI не работает

**Решение**:
```cpp
HTTPServer::start_server(5000);
```

#### 3. **Смешанное использование путей**
- `FileManager` использует XDG (XDG_DATA_HOME) — **новый**
- `DifWork::shift_snapshots()` использует `env::` + `FileAccess::` — **старый**
- **Итог**: миграция незавершена

**Решение**: полностью перейти на `FileManager` API

#### 4. **`src/threads.cpp` - пустой `main()`**
```cpp
int main(){// empty - NO HTTP handlers registered
```

**Итог**: app binary существует, но ничего не делает

---

### 🟡 Средний приоритет

#### 5. `src/main.cpp` - закомментированная старая main()
```cpp
int main(int argc, char** argv){ // commented out
```

#### 6. Python GUI удален на `test-dart-websockets`
- На `main`/`dev` ветке: `gui/gui/main.py` существует
- На `test-dart-websockets`: GUI удален

---

### 🟢 Низкий приоритет

#### 7. C++ тесты отсутствуют
- Только ad-hoc тесты в `main.cpp`
- Нет framework, no CI

#### 8. `DataManager` migration неполная
- `DataManager::get_name()` добавлена
- `DifWork` использует `dm` в конструкторе
- Но `shift_snapshots()` еще использует старый API

---

## 📊 Статус компонентов

| Компонент | Статус | Описание |
|-----------|--------|-----------|
| **C++ Core** | ⚠️ В незавершенном рефакторинге | Смесь старого и нового API, битая миграция |
| **Python GUI** | ❌ Удален | Существовал на `main`/`dev` |
| **Dart CLI** | 🚧 WIP | В разработке |
| **Flutter App** | 🚧 WIP | В разработке |
| **HTTP API** | ❌ Не запускается | Сервер не стартует |
| **TCP Sync** | ✅ Готов | P2P sync работает |
| **UDP Broadcast** | ✅ Готов | Discovery работает |
| **Crypto** | ✅ Готов | X25519/Ed25519 реализованы |

---

## 🚀 Roadmap

### Краткосрочные задачи

1. **Исправить `FileAccess::`** в `DifWork::shift_snapshots()`
2. **Запустить HTTP-сервер** на порту 5000
3. **Забить все `env::`** на `FileManager` API
4. **Добавить HTTP handlers** для GUI endpoints

### Среднесрочные задачи

5. Реализовать GUI handlers:
   - `/devices` — список устройств
   - `/incoming_connect` — приём входящих соединений
   - `/status` — статус sync
   - `/conflicts` — управление конфликтами

6. Настроить C++ тесты (Google Test / Catch2)

### Долгосрочные задачи

7. Завершить Dart CLI
8. Реализовать Flutter app

---

## 📝 Примечание

Текущая ветка: **`test-dart-websockets`** (commit `b7410e0`)

Последний коммит: миграция на XDG_DATA_HOME, удаление SDL2, введение `DataManager`/`DeviceManager`

---

**Status**: ⚠️ **Code doesn't compile or run** due to:
- `FileAccess::` syntax error in `dif_work.cpp`
- Empty `main()` in `threads.cpp`
- HTTP server never started

**Next steps**: Fix migration, enable HTTP API, run tests
