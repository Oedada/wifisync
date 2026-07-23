import 'package:cli/cli.dart';
import 'package:websockets/websockets.dart';

CommandNode buildTree(WS webs) {
  return CommandNode(
    command: CommandNode.rootCmd,
    description: "Wifisync CLI — управление синхронизацией файлов",
    flags: [CliOption(name: "help", abbr: "h")],
    function: ({flags = const {}, options = const {}, description = ""}) {
      print("Wifisync CLI");
      print("Использование: cli <команда> [опции]");
    },
    subCommands: [
      // ── core ──────────────────────────────────────────────
      CommandNode(
        command: "core",
        description: "Управление core-демоном",
        function: ({flags = const {}, options = const {}, description = ""}) {
          print("core");
        },
        subCommands: [
          CommandNode(
            command: "start",
            description: "Запустить WS-сервер, UDP broadcast",
            function:
                ({flags = const {}, options = const {}, description = ""}) {
                  print("core start");
                },
          ),
          CommandNode(
            command: "stop",
            description: "Остановить core-демон",
            function:
                ({flags = const {}, options = const {}, description = ""}) {
                  print("core stop");
                },
          ),
          CommandNode(
            command: "status",
            description: "Статус: alive / connected / idle",
            function:
                ({flags = const {}, options = const {}, description = ""}) {
                  print("core status");
                },
          ),
        ],
      ),
      // ── device ────────────────────────────────────────────
      CommandNode(
        command: "device",
        description: "Управление устройствами",
        function: ({flags = const {}, options = const {}, description = ""}) {
          print("device");
        },
        subCommands: [
          CommandNode(
            command: "list",
            description: "Устройства в сети",
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.requestType,
                        action: "device.list",
                        parameters: {},
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "connect",
            description: "Подключиться к устройству",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.commandType,
                        action: "device.connect",
                        parameters: {"uuid": "1234567890"},
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "accept",
            description: "Принять входящее подключение",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.commandType,
                        action: "device.accept",
                        parameters: {"uuid": "1234567890"},
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "decline",
            description: "Отклонить входящее подключение",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.commandType,
                        action: "device.decline",
                        parameters: {"uuid": "1234567890"},
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "forget",
            description: "Забыть устройство",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function:
                ({flags = const {}, options = const {}, description = ""}) {
                  print("device forget --uuid=${options["uuid"]}");
                },
          ),
          CommandNode(
            command: "know-list",
            description: "Список известных устройств из конфига",
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.requestType,
                        action: "device.know-list",
                        parameters: {},
                      ),
                    ),
                  );
                },
          ),
        ],
      ),
      // ── path ──────────────────────────────────────────────
      CommandNode(
        command: "path",
        description: "Управление путями синхронизации",
        function: ({flags = const {}, options = const {}, description = ""}) {
          print("path");
        },
        subCommands: [
          CommandNode(
            command: "add",
            description: "Добавить путь в синхронизацию",
            options: [
              CliOption(name: "local", abbr: "l", mandatory: true),
              CliOption(name: "remote", abbr: "r", mandatory: true),
              CliOption(name: "device", abbr: "d", mandatory: true),
            ],
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.commandType,
                        action: "path.add",
                        parameters: options,
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "rm",
            description: "Удалить путь из синхронизации",
            options: [CliOption(name: "local", abbr: "l", mandatory: true)],
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.commandType,
                        action: "path.rm",
                        parameters: options,
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "list",
            description: "Список путей синхронизации",
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.requestType,
                        action: "path.list",
                        parameters: {},
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "ignore",
            description: "Управление игнорируемыми паттернами",
            function:
                ({flags = const {}, options = const {}, description = ""}) {
                  print("path ignore");
                },
            subCommands: [
              CommandNode(
                command: "add",
                description: "Добавить паттерн в игнор",
                options: [
                  CliOption(name: "pattern", abbr: "p", mandatory: true),
                ],
                function:
                    ({flags = const {}, options = const {}, description = ""}) {
                      print(
                        webs.send(
                          msg: ActionMsg(
                            type: ActionMsg.commandType,
                            action: "path.ignore.add",
                            parameters: options,
                          ),
                        ),
                      );
                    },
              ),
              CommandNode(
                command: "remove",
                description: "Удалить паттерн из игнора",
                options: [
                  CliOption(name: "pattern", abbr: "p", mandatory: true),
                ],
                function:
                    ({
                      flags = const {},
                      options = const {},
                      description = "",
                    }) async {
                      print(
                        await webs.send(
                          msg: ActionMsg(
                            type: ActionMsg.commandType,
                            action: "path.ignore.rm",
                            parameters: options,
                          ),
                        ),
                      );
                    },
              ),
              CommandNode(
                command: "list",
                description: "Список игнорируемых паттернов",
                function:
                    ({flags = const {}, options = const {}, description = ""}) {
                      print(
                        webs.send(
                          msg: ActionMsg(
                            type: ActionMsg.requestType,
                            action: "path.ignore.list",
                            parameters: options,
                          ),
                        ),
                      );
                    },
              ),
            ],
          ),
        ],
      ),
      // ── sync ──────────────────────────────────────────────
      CommandNode(
        command: "sync",
        description: "Управление синхронизацией",
        function: ({flags = const {}, options = const {}, description = ""}) {
          print("sync");
        },
        subCommands: [
          CommandNode(
            command: "start",
            description: "Запустить синхронизацию с устройством",
            options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.commandType,
                        action: "sync.start",
                        parameters: options,
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "status",
            description: "Прогресс / текущий шаг синхронизации",
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.requestType,
                        action: "sync.status",
                        parameters: options,
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "conflicts",
            description: "Список конфликтов",
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.requestType,
                        action: "sync.conflicts",
                        parameters: options,
                      ),
                    ),
                  );
                },
          ),
          CommandNode(
            command: "cancel",
            description: "Прервать текущий sync",
            function:
                ({
                  flags = const {},
                  options = const {},
                  description = "",
                }) async {
                  print(
                    await webs.send(
                      msg: ActionMsg(
                        type: ActionMsg.commandType,
                        action: "sync.cancel",
                        parameters: options,
                      ),
                    ),
                  );
                },
          ),
        ],
      ),
    ],
  );
}

Future<void> main(List<String> arguments) async {
  var ws = await WS.connect(port: 9001, onInfo: (InfoMsg msg) => print(msg));
  var root = buildTree(ws);
  var pars = Parser(rootCmdNode: root);
  await pars.processCommand(arguments)();
  // await ws.close();
}
