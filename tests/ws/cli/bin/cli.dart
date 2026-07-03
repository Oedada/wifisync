import 'package:args/args.dart';

typedef CliFunction = Function({
  required Map<String, bool> flags,
  required Map<String, String?> options,
  required String description,
});

void log({
  required Map<String, bool> flags,
  required Map<String, String?> options,
  required String description,
}) {
  print("Flags: $flags, Options: $options, description: $description");
}
void core_log({
  required Map<String, bool> flags,
  required Map<String, String?> options,
  required String description,
}) {
  print("Core log");
}

class CliItem {
  final String name;
  final String abbr;
  CliItem({required this.name, required this.abbr});
}

class CliOption extends CliItem {
  final bool mandatory;
  CliOption({required super.name, required super.abbr, this.mandatory = false});
}

class CommandNode {
  static const rootCmd = "/";
  final String command;
  final List<CliOption> flags;
  final List<CliOption> options;
  final List<CommandNode> subCommands;
  final CliFunction function;
  final String description;
  final Map<String, CommandNode> cmdDict = {};
  CommandNode({
    required this.command,
    required this.function,
    this.subCommands = const [],
    this.options = const [],
    this.flags = const [],
    this.description = "",
  }) {
    for (final cmd in subCommands) {
      cmdDict[cmd.command] = cmd;
    }
  }
  CommandNode? getCmd({String? name}) {
    if (name == null) {
      throw Exception("Name of command should be not null");
    }
    var cmd = cmdDict[name];
    return cmd;
  }
}

class Parser {
  final CommandNode rootCmdNode;
  ArgParser argP;
  Parser({required this.rootCmdNode})
    : argP = _setArgParser(rootCmdNode, ArgParser());

  static ArgParser _setArgParser(CommandNode cmdNode, ArgParser ap) {
    for (final flag in cmdNode.flags) {
      ap.addFlag(flag.name, abbr: flag.abbr);
    }
    for (final option in cmdNode.options) {
      ap.addOption(option.name, abbr: option.abbr);
    }
    for (final cmd in cmdNode.subCommands) {
      ap.addCommand(cmd.command, _setArgParser(cmd, ArgParser()));
    }
    return ap;
  }

  Function processCommand(List<String> arguments) {
    final results = argP.parse(arguments);
    return _findFunction(results, rootCmdNode, results);
  }

  Function _findFunction(
    ArgResults cmd,
    CommandNode cmdNode,
    ArgResults results,
  ) {
    final Map<String, bool> flags = {};
    final Map<String, String?> options = {};
    for (final flag in cmdNode.flags) {
      flags[flag.name] = results[flag.name];
    }
    for (final option in cmdNode.options) {
      options[option.name] = results[option.name];
    }
    var subCmd = cmd.command;
    if (cmdNode.subCommands.isEmpty || subCmd == null) {
      return () => cmdNode.function(
        flags: flags,
        options: options,
        description: cmdNode.description,
      );
    }
    CommandNode? subCmdNode;
    subCmdNode = cmdNode.getCmd(name: subCmd.name);
    if (subCmdNode == null) {
      return () => cmdNode.function(
        flags: flags,
        options: options,
        description: cmdNode.description,
      );
    }
    return _findFunction(subCmd, subCmdNode, results);
  }
}

CommandNode cde = CommandNode(
  command: CommandNode.rootCmd,
  flags: [CliOption(name: "help", abbr: "h")],
  subCommands: [
    CommandNode(
      command: "core",
      subCommands: [
        CommandNode(command: "start", function: log),
        CommandNode(command: "stop", function: log),
      ],
      function: core_log,
    ),
  ],
  function: log,
);

void main(List<String> arguments) {
  var pars = Parser(rootCmdNode: cde);
  pars.processCommand(arguments)();
}
