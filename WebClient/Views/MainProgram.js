var net = require('net');

function initialize() {
	//  Create the base container that the program will exist in
	container = document.createElement("div");
	document.body.appendChild(container);
	
	let testElement = document.createElement("div");
	testElement.style.width = "100px";
	testElement.style.height = "100px";
	testElement.style.backgroundColor = "black";
	container.appendChild(testElement);
	
	var client = new net.Socket();
	client.connect(4567, '127.0.0.1', function() {
		console.log('Connected');
		client.write('Hello, server! Love, Client.');
	});
	
	client.on('data', function(data) {
		console.log('Received: ' + data);
		client.destroy(); // kill client after server's response
	});
	
	client.on('close', function() {
		console.log('Connection closed');
	});
}

//  Run the initialize function above
initialize();