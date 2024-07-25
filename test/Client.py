import asyncio
import websockets
import socket

async def receive_data():
    uri = f"ws://{socket.gethostbyname('DipMachine.local')}/ws"
    async with websockets.connect(uri) as websocket:
        data = await websocket.recv()
        print(f"Received data: {data}")
        await websocket.send("Connected")

asyncio.get_event_loop().run_until_complete(receive_data())
