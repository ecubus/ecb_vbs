const socket = new WebSocket('ws://localhost:20101');
// Executes when the connection is successfully established.
let id = 0;
socket.addEventListener('open', event => {
    console.log('WebSocket connection established!');
    // Sends a message to the WebSocket server.
    socket.send(JSON.stringify({
        method: 'write',
        topic_name: "domain_0_topic_0Topic",
        type_name:"MVBS_HelloWorld",
        data: {
           id:5,
           msg: 'Hello World!',
        },
        id:id++
    }));
    socket.send(JSON.stringify({
        method: 'get_writer_peers',
        topic_name: "domain_0_topic_0Topic",
        id:id++
    }));
    
});
// Listen for messages and executes when a message is received from the server.
socket.addEventListener('message', event => {
    console.log('Message from server: ', event.data);
});
// Executes when the connection is closed, providing the close code and reason.
socket.addEventListener('close', event => {
    console.log('WebSocket connection closed:', event.code, event.reason);
});
// Executes if an error occurs during the WebSocket communication.
socket.addEventListener('error', error => {
    console.error('WebSocket error:', error);
});