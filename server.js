const WebSocket = require('ws');
const { spawn } = require('child_process');
const readline = require('readline');

// Spawn the ./bitlite process to get the phone's IP and port
const bitliteProcess = spawn('./build/bitlite', [], { stdio: ['pipe', 'pipe', 'pipe'] });

if (!bitliteProcess) {
    console.error('Failed to start the bitlite process. Ensure the binary exists and is executable.');
    process.exit(1);
}

let phoneIp = '';
let phonePort = 0;

// Capture the output from the bitlite process
bitliteProcess.stdout.on('data', (data) => {
    const output = data.toString();
    console.log(`bitlite: ${output}`);

    // Extract the IP and port from the output
    const ipMatch = output.match(/Phone IP: ([0-9.]+)/);
    const portMatch = output.match(/Port: ([0-9]+)/);

    if (ipMatch && portMatch) {
        phoneIp = ipMatch[1];
        phonePort = parseInt(portMatch[1], 10);

        // Connect to the WebSocket server on the phone
        const ws = new WebSocket(`ws://${phoneIp}:${phonePort}`);

        ws.on('open', () => {
            console.log(`Connected to WebSocket server at ws://${phoneIp}:${phonePort}`);

            ws.send('Hello from the laptop!');

            // Allow user to send custom messages
            rl.on('line', (input) => {
                ws.send(input);
            });
        });

        ws.on('message', (message) => {
            console.log(`Received from server: ${message}`);
        });

        ws.on('close', () => {
            console.log('Connection to WebSocket server closed.');
        });

        ws.on('error', (error) => {
            console.error(`WebSocket error: ${error.message}`);
        });
    }
});

bitliteProcess.stderr.on('data', (data) => {
    console.error(`bitlite error: ${data}`);
});

bitliteProcess.on('close', (code) => {
    console.log(`bitlite process exited with code ${code}`);
});

// Forward user input to the bitlite process
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

rl.on('line', (input) => {
    bitliteProcess.stdin.write(`${input}\n`);
});