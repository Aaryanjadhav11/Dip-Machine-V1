import asyncio
import websockets
import socket
import json

status = ["new", "abort", "recover"]
msg = {
    "status" : "",
    "activeBeakers" : 6,
    "setCycles" : 10,
    "setDipTemperature" : [50, 50, 50, 50, 50, 50],
    "setDipDuration" : [1,1,1,1,1,1],
    "setDipRPM" : [300, 300, 300, 300, 300, 300]
}
async def receive_data():
    try:
        # Resolve the local IP address of the server
        ip_address = socket.gethostbyname('DipMachine.local')
        uri = f"ws://{ip_address}/ws"
        async with websockets.connect(uri) as websocket:
            print(await websocket.recv())
            for i, state in enumerate(status, 1):
                print(f"{i}. {state}")
            msg["status"] = status[int(input("Enter the number of your choice: "))-1]
            print(msg)
            await websocket.send(json.dumps(msg))
            while True:
                print(await websocket.recv())
    except socket.gaierror:
        print("Could not resolve the server address.")
    except websockets.ConnectionClosedError:
        print("Failed to connect to the server.")
    except json.JSONDecodeError:
        print("Failed to decode the JSON response.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Run the asynchronous function
asyncio.run(receive_data())
