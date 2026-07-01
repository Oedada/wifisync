import 'dart:io';

Future<void> main() async {
  final ws = await WebSocket.connect('ws://127.0.0.1:12346');

  ws.add('Привет');

  await for (final msg in ws) {
    print('Получено: $msg');
  }
}
