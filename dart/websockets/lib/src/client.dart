import 'dart:async';
import 'dart:io';
import 'dart:convert';

typedef Json = Map<String, dynamic>;

T checkField<T>(Json json, String key) {
  final field = json[key];
  if (field is T) {
    return field;
  } else {
    throw FormatException(
      "The \"$key\" field should be a $T, but it is ${field.runtimeType}",
    );
  }
}

class InfoMsg {
  final String event;
  final Json parameters;

  InfoMsg({required this.event, required this.parameters});

  factory fromJson(Json json) {
    return InfoMsg(
      event: checkField<String>(json, "event"),
      parameters: checkField<Json>(json, "parameters"),
    );
  }
}

class RespMsg {
  final Json parameters;
  final bool status;
  static const String type = "resp";
  RespMsg({required this.parameters, required this.status});

  factory fromJson(Json json) {
    return RespMsg(
      parameters: checkField<Json>(json, "parameters"),
      status: checkField<bool>(json, "ok"),
    );
  }
}

class ActionMsg {
  static const requestType = "req";
  static const commandType = "cmd";
  final String type;
  final String action;
  final Json parameters;
  ActionMsg({
    required this.type,
    required this.action,
    required this.parameters,
  });

  Json toJson() {
    Json json = {};
    json["type"] = type;
    json["action"] = action;
    json["parameters"] = parameters;
    return json;
  }

  factory fromJson(Json json) {
    final String type;
    final String action;
    final Json parameters;
    if (json["type"] == requestType || json["type"] == commandType) {
      type = json["type"];
    } else {
      throw FormatException(
        "The type field must be either \"$commandType\" or \"$requestType\"",
      );
    }
    action = checkField<String>(json, "action");
    parameters = checkField<Json>(json, "parameters");
    return ActionMsg(type: type, action: action, parameters: parameters);
  }
}

class WS {
  final WebSocket _ws;
  int _lid = 0;
  final Map<int, Completer<RespMsg>> _pendings = {};
  final Function(InfoMsg) onInfo;

  WS._(this._ws, this.onInfo) {
    _listenMessages();
  }

  static Future<WS> connect({
    required int port,
    Function(InfoMsg)? onInfo,
  }) async {
    onInfo ??= (_) {};
    return WS._(await WebSocket.connect('ws://127.0.0.1:$port/ws'), onInfo);
  }

  Future<void> _listenMessages() async {
    try {
      await for (final msg in _ws) {
        if (msg is String) {
          var json = jsonDecode(msg);
          final id = json["id"];
          if (id is int) {
            if (id < 0) {
              onInfo(InfoMsg.fromJson(json));
            } else if (_pendings.containsKey(id)) {
              _pendings[id]!.complete(RespMsg.fromJson(json));
              _pendings.remove(id);
            }
          } else {
            throw ArgumentError("Id should be int");
          }
        } else {
          throw ArgumentError("Message should be string");
        }
      }
    } finally {
      for (final entry in _pendings.entries) {
        entry.value.completeError("WS closed");
      }
      _pendings.clear();
    }
  }

  Future<RespMsg> send({required ActionMsg msg}) async {
    int id = ++_lid;
    final json = msg.toJson();
    json["id"] = id;
    _ws.add(jsonEncode(json));
    var completer = Completer<RespMsg>();
    _pendings[id] = completer;
    return completer.future;
  }

  Future<void> close() async {
    await _ws.close();
  }
}

Future<void> main() async {
  var infoEvents = <InfoMsg>[];
  var ws = await WS.connect(
    port: 9001,
    onInfo: (msg) {
      infoEvents.add(msg);
      print("info: ${msg.event} ${msg.parameters}");
    },
  );

  // 1. connect
  var r1 = await ws.send(
    msg: ActionMsg(type: "cmd", action: "device.connect", parameters: {"uuid": "abc-123"}),
  );
  print("connect ok: ${r1.status} parameters: ${r1.parameters}");

  // 2. list
  var r2 = await ws.send(
    msg: ActionMsg(type: ActionMsg.requestType, action: "device.list", parameters: {}),
  );
  print("list ok: ${r2.status} devices: ${r2.parameters["devices"]}");

  // 3. error
  var r3 = await ws.send(
    msg: ActionMsg(type: ActionMsg.commandType, action: "error.test", parameters: {}),
  );
  print("error ok: ${r3.status}");

  // 4. concurrent
  var futures = [
    ws.send(msg: ActionMsg(type: "req", action: "device.list", parameters: {})),
    ws.send(msg: ActionMsg(type: "cmd", action: "device.connect", parameters: {"uuid": "xyz"})),
  ];
  var results = await Future.wait(futures);
  print("concurrent: ${results[0].status} ${results[1].status}");

  print("info events received: ${infoEvents.length}");
  await ws.close();
}
