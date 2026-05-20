import 'package:flutter/material.dart';
import 'dart:convert';

class User {
  String name;
  Map<String, String> paths;

  User({required this.name, required this.paths});

  factory User.fromJson(Map<dynamic, dynamic> json) {
    return User(
      name: json["name"] as String,
      paths: Map<String, String>.from(json["paths"]),
    );
  }
}

class Users {
  Map<String, User> users;
  Users({required this.users});

  factory Users.fromJson(Map<String, dynamic> json) {
    final Map<String, User> usrs = {};
    json.forEach((uuid, val) {
      usrs[uuid] = User.fromJson(val);
    });
    return Users(users: usrs);
  }
}

void main() {
  runApp(WSApp());
}

class WSApp extends StatelessWidget {
  const WSApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(home: TabsPage());
  }
}

class TabsPage extends StatelessWidget {
  const TabsPage({super.key});

  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
      length: 2,
      child: Scaffold(
        appBar: AppBar(
          title: const Text("Wifisync"),
          bottom: const TabBar(
            tabs: [
              Tab(icon: Icon(Icons.home)),
              Tab(icon: Icon(Icons.settings)),
            ],
          ),
        ),
        body: TabBarView(
          children: [
            Center(child: TextPage()),
            Center(child: SettingsPage()),
          ],
        ),
      ),
    );
  }
}

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  final jsonString = '''{
    "uuid1":{
      "name": "oedada",
      "paths": {
        "/home/local": "/home/other",
        "/home/etc": "/home/etc",
        "/home/bin": "/home/oedada/backup/bin"
      }
    }
  }''';

  Column getPaths() {
    final users = Users.fromJson(jsonDecode(jsonString));
    return Column(
      children: [
        for (final entry in users.users.entries)
          ExpansionTile(
            title: Text(entry.value.name),
            children: [
              for (final e in entry.value.paths.entries)
                ListTile(leading: Text(e.key), trailing: Text(e.value)),
            ],
          ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(body: getPaths());
  }
}

class TextPage extends StatefulWidget {
  const TextPage({super.key});

  @override
  State<TextPage> createState() => _TextPageState();
}

class _TextPageState extends State<TextPage> {
  String _text = "";

  void onTextChanged(String txt) {
    setState(() {
      _text = txt;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Текст")),
      body: Center(
        child: Column(
          mainAxisAlignment: .center,
          children: [
            TextField(onChanged: onTextChanged),
            Text(_text),
          ],
        ),
      ),
    );
  }
}
