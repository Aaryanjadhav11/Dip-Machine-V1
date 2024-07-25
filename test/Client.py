import asyncio
import websockets
import socket
import json

msg = {
    "status" : 1
}

async def receive_data():
    uri = f"ws://{socket.gethostbyname('DipMachine.local')}/ws"
    async with websockets.connect(uri) as websocket:
       pass
asyncio.get_event_loop().run_until_complete(receive_data())
