import 'package:cli/cli.dart';

CommandNode buildTree() {
  return CommandNode(
    command: CommandNode.rootCmd,
    description: "Wifisync CLI — управление синхронизацией файлов",
    flags: [CliOption(name: "help", abbr: "h")],
    function: create_cli_function(({flags, options, description}) {
      print("Wifisync CLI");
      print("Использование: cli <команда> [опции]");
    }),
    subCommands: [
      // ── core ──────────────────────────────────────────────
      CommandNode(
        command: "core",
        description: "Управление core-демоном",
        function: create_cli_function(({flags, options, description}) {
          print("core");
        }),
        subCommands: [
          CommandNode(
            command: "start",
            description: "Запустить WS-сервер, UDP broadcast",
            function: create_cli_function(({flags, options, description}) {
              print("core start");
            }),
          ),
          CommandNode(
            command: "stop",
            description: "Остановить core-демон",
            function: create_cli_function(({flags, options, description}) {
              print("core stop");
            }),
          ),
          CommandNode(
            command: "status",
            description: "Статус: alive / connected / idle",
            function: create_cli_function(({flags, options, description}) {
              print("core status");
            }),
          ),
        ],
      ),
      // ── device ────────────────────────────────────────────
      CommandNode(
        command: "device",
        description: "Управление устройствами",
        function: create_cli_function(({flags, options, description}) {
          print("device");
        }),
        subCommands: [
          CommandNode(
            command: "list",
            description: "Устройства в сети",
            function: create_cli_function(({flags, options, description}) {
              print("device list");
            }),
          ),
          CommandNode(
            command: "connect",
            description: "Подключиться к устройству",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function: create_cli_function(({flags, options, description}) {
              print("device connect --uuid=${options["uuid"]}");
            }),
          ),
          CommandNode(
            command: "accept",
            description: "Принять входящее подключение",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function: create_cli_function(({flags, options, description}) {
              print("device accept --uuid=${options["uuid"]}");
            }),
          ),
          CommandNode(
            command: "reject",
            description: "Отклонить входящее подключение",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function: create_cli_function(({flags, options, description}) {
              print("device reject --uuid=${options["uuid"]}");
            }),
          ),
          CommandNode(
            command: "forget",
            description: "Забыть устройство",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function: create_cli_function(({flags, options, description}) {
              print("device forget --uuid=${options["uuid"]}");
            }),
          ),
          CommandNode(
            command: "know-list",
            description: "Список известных устройств из конфига",
            function: create_cli_function(({flags, options, description}) {
              print("device know-list");
            }),
          ),
        ],
      ),
      // ── path ──────────────────────────────────────────────
      CommandNode(
        command: "path",
        description: "Управление путями синхронизации",
        function: create_cli_function(({flags, options, description}) {
          print("path");
        }),
        subCommands: [
          CommandNode(
            command: "add",
            description: "Добавить путь в синхронизацию",
            options: [
              CliOption(name: "local", abbr: "l", mandatory: true),
              CliOption(name: "remote", abbr: "r", mandatory: true),
              CliOption(name: "device", abbr: "d", mandatory: true),
            ],
            function: create_cli_function(({flags, options, description}) {
              print("path add --local=${options["local"]} --remote=${options["remote"]} --device=${options["device"]}");
            }),
          ),
          CommandNode(
            command: "remove",
            description: "Удалить путь из синхронизации",
            options: [CliOption(name: "local", abbr: "l", mandatory: true)],
            function: create_cli_function(({flags, options, description}) {
              print("path remove --local=${options["local"]}");
            }),
          ),
          CommandNode(
            command: "list",
            description: "Список путей синхронизации",
            function: create_cli_function(({flags, options, description}) {
              print("path list");
            }),
          ),
          CommandNode(
            command: "ignore",
            description: "Управление игнорируемыми паттернами",
            function: create_cli_function(({flags, options, description}) {
              print("path ignore");
            }),
            subCommands: [
              CommandNode(
                command: "add",
                description: "Добавить паттерн в игнор",
                options: [CliOption(name: "pattern", abbr: "p", mandatory: true)],
                function: create_cli_function(({flags, options, description}) {
                  print("path ignore add --pattern=${options["pattern"]}");
                }),
              ),
              CommandNode(
                command: "remove",
                description: "Удалить паттерн из игнора",
                options: [CliOption(name: "pattern", abbr: "p", mandatory: true)],
                function: create_cli_function(({flags, options, description}) {
                  print("path ignore remove --pattern=${options["pattern"]}");
                }),
              ),
              CommandNode(
                command: "list",
                description: "Список игнорируемых паттернов",
                function: create_cli_function(({flags, options, description}) {
                  print("path ignore list");
                }),
              ),
            ],
          ),
        ],
      ),
      // ── sync ──────────────────────────────────────────────
      CommandNode(
        command: "sync",
        description: "Управление синхронизацией",
        function: create_cli_function(({flags, options, description}) {
          print("sync");
        }),
        subCommands: [
          CommandNode(
            command: "start",
            description: "Запустить синхронизацию с устройством",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function: create_cli_function(({flags, options, description}) {
              print("sync start --uuid=${options["uuid"]}");
            }),
          ),
          CommandNode(
            command: "status",
            description: "Прогресс / текущий шаг синхронизации",
            function: create_cli_function(({flags, options, description}) {
              print("sync status");
            }),
          ),
          CommandNode(
            command: "conflicts",
            description: "Список конфликтов",
            function: create_cli_function(({flags, options, description}) {
              print("sync conflicts");
            }),
          ),
          CommandNode(
            command: "cancel",
            description: "Прервать текущий sync",
            function: create_cli_function(({flags, options, description}) {
              print("sync cancel");
            }),
          ),
        ],
      ),
    ],
  );
}

void main(List<String> arguments) {
  var root = buildTree();
  var pars = Parser(rootCmdNode: root);
  pars.processCommand(arguments)();
}
