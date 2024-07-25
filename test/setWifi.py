import asyncio
import websockets
import socket
import json

# Message to initiate WiFi scanning

async def receive_data():
    try:
        # Resolve the local IP address of the server
        ip_address = socket.gethostbyname('DipMachine.local')
        uri = f"ws://{ip_address}/ws"

        async with websockets.connect(uri) as websocket:
            # Send the initial scanWiFi message
            await websocket.send(json.dumps(dict(status = "scanWiFi")))

            # Receive the list of available networks
            print("\nSearching for newtorks, Hang on...\n")
            response = await websocket.recv()
            message = json.loads(response)

            # Display the available networks
            ssids = []
            for i, ssid in enumerate(message):
                network_info = message[ssid]
                is_open = " " if network_info["isOpen"] else "*"
                print(f"[{i + 1}] SSID: {ssid} {is_open} {network_info['rssi']}")
                ssids.append(ssid)

            # Prompt the user to select a network
            while True:
                try:
                    tag = int(input(f"Select a network you want to set [1-{len(ssids)}]: ")) - 1
                    if 0 <= tag < len(ssids):
                        break
                    else:
                        print(f"Please enter a number between 1 and {len(ssids)}.")
                except ValueError:
                    print("Invalid input. Please enter a number.")

            # Update the message to set WiFi
            selected_ssid = ssids[tag]
            passw = "" if message[selected_ssid]["isOpen"] else input(f"Enter the password for [{selected_ssid}]: ")
            msg = dict(status = "setWiFi", ssid = selected_ssid, password = passw)

            # Send the updated message to set the WiFi
            await websocket.send(json.dumps(msg))
            print("WiFi credentials sent to the server.")

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
