# chat_server.py - Corrected for bidirectional chat from phone (Python 3.11+ asyncio compatibility)

import asyncio
import websockets
import logging
import sys

# Configure logging to show info messages
logging.basicConfig(level=logging.INFO, stream=sys.stdout)

# Set to store all connected WebSocket clients
CONNECTED_CLIENTS = set()

async def register(websocket):
    """Registers a new client connection."""
    CONNECTED_CLIENTS.add(websocket)
    logging.info(f"Client connected: {websocket.remote_address}. Total clients: {len(CONNECTED_CLIENTS)}")
    # Optional: Send a welcome message to the newly connected client
    await websocket.send("Server: Welcome! You are now connected.")

async def unregister(websocket):
    """Unregisters a disconnected client."""
    CONNECTED_CLIENTS.remove(websocket)
    logging.info(f"Client disconnected: {websocket.remote_address}. Total clients: {len(CONNECTED_CLIENTS)}")

async def broadcast_message(message: str, sender_websocket=None):
    """
    Sends a message to all connected clients.
    If sender_websocket is provided, it's a client's message.
    If sender_websocket is None, it's a server-originated message.
    """
    # Determine target clients and format message
    if sender_websocket:
        formatted_message = f"[{sender_websocket.remote_address}]: {message}"
        log_prefix = f"Broadcasting from {sender_websocket.remote_address}"
        target_websockets = list(CONNECTED_CLIENTS)
    else:
        formatted_message = f": {message}"
        log_prefix = "Broadcasting from server"
        target_websockets = list(CONNECTED_CLIENTS)  # Send to ALL connected clients if from server

    # FIX: Explicitly create tasks for each send operation
    tasks = [asyncio.create_task(ws.send(formatted_message)) for ws in target_websockets]

    if tasks:  # Only proceed if there are actual tasks to wait for
        logging.info(f"{log_prefix}: {formatted_message}")

        # Send to all relevant clients concurrently
        done, pending = await asyncio.wait(tasks)

        # Optionally, check for exceptions in the completed tasks
        for task in done:
            if task.exception():
                logging.error(f"Error sending message to a client: {task.exception()}")
    else:
        logging.info("No target clients to broadcast to.")  # No clients or only sender present

async def chat_handler(websocket):
    """Handles individual client WebSocket connections (receiving messages)."""
    await register(websocket)
    try:
        async for message in websocket:
            logging.info(f"Received message from {websocket.remote_address}: {message}")
            # Echo message back to sender (optional, can be removed if not desired)
            await websocket.send(f"Server received: {message}")
            # Broadcast to others
            await broadcast_message(message, websocket)  # Pass the actual client websocket
    except websockets.exceptions.ConnectionClosedOK:
        logging.info(f"Client {websocket.remote_address} closed connection gracefully.")
    except websockets.exceptions.ConnectionClosedError as e:
        logging.error(f"Client {websocket.remote_address} connection closed with error: {e}")
    except Exception as e:
        logging.error(f"An unexpected error occurred with {websocket.remote_address}: {e}", exc_info=True)
    finally:
        await unregister(websocket)

async def server_input_loop():
    """Reads input from the server's stdin and broadcasts it to clients."""
    print("\nServer: Type message to send to clients (or Ctrl+C to stop server):")
    while True:
        try:
            # asyncio.to_thread runs blocking code (like input()) in a separate thread
            # so it doesn't block the main async event loop.
            message = await asyncio.to_thread(sys.stdin.readline)
            message = message.strip()  # Remove newline character

            if message:  # Don't send empty messages
                await broadcast_message(message, sender_websocket=None)  # No sender_websocket means it's from server
        except Exception as e:
            logging.error(f"Error in server input loop: {e}", exc_info=True)
            break  # Exit loop on error

async def main():
    """Starts the WebSocket server and the server input loop."""
    host = "0.0.0.0"  # Listen on all available interfaces
    port = 6000  # Use the same port as before

    logging.info(f"Starting WebSocket server on ws://{host}:{port}")

    # Start the WebSocket server itself using async with
    async with websockets.serve(chat_handler, host, port) as server:
        # Start the task for reading server input
        input_task = asyncio.create_task(server_input_loop())

        # Run both tasks concurrently.
        # FIX: Explicitly create a task for server.wait_closed()
        try:
            await asyncio.gather(
                input_task,
                asyncio.create_task(server.wait_closed())  # <-- FIX: Wrap coroutine in create_task
            )
        except Exception as e:
            logging.critical(f"Server encountered a critical error: {e}", exc_info=True)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("Server stopped by user via Ctrl+C.")
    except Exception as e:
        logging.critical(f"Server crashed: {e}", exc_info=True)