curl -X POST http://0.0.0.0:5000/tasks -H "Content-Type: application/json" -d '{"cmd":"connect","args":["192.168.0.104",12345]}'

curl -X POST http://192.168.0.105:5000/tasks -H "Content-Type: application/json" -d '{"cmd":"connect","args":["192.168.0.104",12345]}'
