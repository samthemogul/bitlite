

import asyncio
import websockets
import logging
import sys


logging.basicConfig(level=logging.INFO, stream=sys.stdout)


CONNECTED_CLIENTS = set()

async def register(websocket):
    """Registers a new client connection."""
    CONNECTED_CLIENTS.add(websocket)
    logging.info(f"Client connected: {websocket.remote_address}. Total clients: {len(CONNECTED_CLIENTS)}")
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
    
    if sender_websocket:
        formatted_message = f"[{sender_websocket.remote_address}]: {message}"
        log_prefix = f"Broadcasting from {sender_websocket.remote_address}"
        target_websockets = list(CONNECTED_CLIENTS)
    else:
        formatted_message = f": {message}"
        log_prefix = "Broadcasting from server"
        target_websockets = list(CONNECTED_CLIENTS)

   
    tasks = [asyncio.create_task(ws.send(formatted_message)) for ws in target_websockets]

    if tasks: 
        logging.info(f"{log_prefix}: {formatted_message}")

       
        done, pending = await asyncio.wait(tasks)

       
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
            await websocket.send(f"Server received: {message}")
            
            # Broadcast to others
            await broadcast_message(message, websocket) 
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
            message = await asyncio.to_thread(sys.stdin.readline)

            if message:
                await broadcast_message(message, sender_websocket=None)
        except Exception as e:
            logging.error(f"Error in server input loop: {e}", exc_info=True)
            break 

async def main():
    """Starts the WebSocket server and the server input loop."""
    host = "0.0.0.0" 
    port = 6000 

    logging.info(f"Starting WebSocket server on ws://{host}:{port}")

    
    async with websockets.serve(chat_handler, host, port) as server:
        input_task = asyncio.create_task(server_input_loop())

        try:
            await asyncio.gather(
                input_task,
                asyncio.create_task(server.wait_closed())
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