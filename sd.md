cli
├── core
│   ├── start              # запустить WS-сервер, UDP broadcast, фоновый процесс
│   ├── stop               # остановить
│   └── status             # alive? connected? idle?
│
├── device
│   ├── list               # устройства в сети
│   ├── connect <uuid>     # установить TCP-соединение
│   ├── accept <uuid>      # принять входящий connect
│   ├── reject <uuid>      # отклонить
│   └── forget <uuid>      # забыть устройство
|   |---know-list # знакомые устройства, сохранённые в конфиге
│
├── path                   # конфиг живёт в core, мы только читаем/пишем через команды
│   ├── add <local> [--remote <path>] [--device <uuid>]
│   ├── remove <local>
│   ├── list
│   ├── ignore
│   │   ├── add <pattern>
│   │   ├── remove <pattern>
│   │   └── list
│
└── sync
    ├── start <uuid>       # запустить sync (устройство уже должно быть connected)
    ├── status             # прогресс / текущий шаг (0–6)
    ├── conflicts          # список конфликтов
    └── cancel             # прервать текущий sync 
Вот, вот такие, пожалуйста реализуй в cli.dart в bin
