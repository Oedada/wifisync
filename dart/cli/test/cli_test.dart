import 'package:cli/cli.dart';
import 'package:test/test.dart';

void main() {
  test('parser walks nested subcommands', () {
    var tree = CommandNode(
      command: CommandNode.rootCmd,
      function: create_cli_function(({flags, options, description}) {}),
      subCommands: [
        CommandNode(
          command: "device",
          function: create_cli_function(({flags, options, description}) {}),
          subCommands: [
            CommandNode(
              command: "list",
              function: create_cli_function(({flags, options, description}) {
                expect(flags["help"], false);
              }),
            ),
            CommandNode(
              command: "connect",
              options: [CliOption(name: "uuid", abbr: "u", mandatory: true)],
              function: create_cli_function(({flags, options, description}) {
                expect(options["uuid"], "abc-123");
              }),
            ),
          ],
        ),
      ],
      flags: [CliOption(name: "help", abbr: "h")],
    );

    var p = Parser(rootCmdNode: tree);
    p.processCommand(["device", "list"])();
    p.processCommand(["device", "connect", "--uuid=abc-123"])();
  });
}
