const WebSocket = require('ws');

const server = new WebSocket.Server({ port: 12346});

server.on('connection', ws => {
  ws.on('message', msg => {
    console.log(msg.toString());
    ws.send(`echo: ${msg}`);
  });
});
