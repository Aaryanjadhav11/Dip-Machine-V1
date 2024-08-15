import asyncio
import websockets
import socket
import json
import tkinter as tk
from tkinter import ttk
import threading

class WebSocketUI:
    def __init__(self, master):
        self.master = master
        master.title("WebSocket Communication UI")
        self.websocket = None
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.status = ["new","start", "abort", "recover"]
        self.msg = {
            "status": "", "activeBeakers": 1, "setCycles": 1,
            "setDipTemperature": [0, 0, 0, 0, 0, 0],
            "setDipDuration": [1, 1, 1, 1, 1, 1],
            "setDipRPM": [0, 0, 0, 0, 0, 0]
        }
        self.setup_ui()
        self.setup_websocket()

    def setup_ui(self):
        self.create_status_frame()
        self.create_input_frame()
        self.create_choice_frame()
        self.create_info_frame()
        self.create_data_frame()

    def create_status_frame(self):
        self.status_frame = ttk.Frame(self.master)
        self.status_frame.pack(fill=tk.X, padx=10, pady=5)
        self.connection_status_label = tk.Label(self.status_frame, text="Connection: Disconnected", 
                                                font=("Arial", 12, "bold"), fg="red")
        self.connection_status_label.pack(side=tk.LEFT, padx=(0, 20))
        self.server_state_label = tk.Label(self.status_frame, text="Server State: Unknown", 
                                           font=("Arial", 12, "bold"), fg="blue")
        self.server_state_label.pack(side=tk.LEFT)

    def create_input_frame(self):
        self.input_frame = ttk.LabelFrame(self.master, text="Machine Input")
        self.input_frame.pack(fill=tk.X, padx=10, pady=5)
        inputs = [("Active Beakers", "activeBeakers", 1, 6),
                  ("Set Cycles", "setCycles", 1, 100)]
        for i, (label, key, min_val, max_val) in enumerate(inputs):
            ttk.Label(self.input_frame, text=label).grid(row=i, column=0, padx=5, pady=2, sticky="e")
            spinbox = ttk.Spinbox(self.input_frame, from_=min_val, to=max_val, width=5)
            spinbox.grid(row=i, column=1, padx=5, pady=2, sticky="w")
            spinbox.set(self.msg[key])
            setattr(self, f"{key}_spinbox", spinbox)
        self.create_array_inputs("Set Dip Temperature", "setDipTemperature", 0, 100)
        self.create_array_inputs("Set Dip Duration", "setDipDuration", 1, 60)
        self.create_array_inputs("Set Dip RPM", "setDipRPM", 0, 600)

    def create_array_inputs(self, label, key, min_val, max_val):
        frame = ttk.Frame(self.input_frame)
        frame.grid(columnspan=2, pady=5)
        ttk.Label(frame, text=label).grid(row=0, column=0, columnspan=6)
        spinboxes = []
        for i in range(6):
            spinbox = ttk.Spinbox(frame, from_=min_val, to=max_val, width=5)
            spinbox.grid(row=1, column=i, padx=2)
            spinbox.set(self.msg[key][i])
            spinboxes.append(spinbox)
        setattr(self, f"{key}_spinboxes", spinboxes)

    def create_choice_frame(self):
        self.choice_frame = ttk.Frame(self.master)
        self.choice_frame.pack(fill=tk.X, padx=10, pady=5)
        self.choice_label = ttk.Label(self.choice_frame, text="Select an option:")
        self.choice_label.pack(side=tk.LEFT)
        self.choice_combobox = ttk.Combobox(self.choice_frame, state="readonly")
        self.choice_combobox.pack(side=tk.LEFT, padx=5)
        self.submit_choice_button = ttk.Button(self.choice_frame, text="Submit", command=self.submit_choice)
        self.submit_choice_button.pack(side=tk.LEFT)

    def create_info_frame(self):
        self.info_frame = ttk.Frame(self.master)
        self.info_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        self.create_machine_section()
        self.create_beaker_section()

    def create_machine_section(self):
        self.machine_frame = ttk.LabelFrame(self.info_frame, text="Machine Information")
        self.machine_frame.grid(row=0, column=0, padx=5, pady=5, sticky="nsew")
        self.machine_params = {}
        params = [("On Cycle", "onCycle"), ("Time Left", "timeLeft"), 
                  ("On Beaker", "onBeaker"), ("Active Beakers", "activeBeakers")]
        for i, (label, key) in enumerate(params):
            ttk.Label(self.machine_frame, text=label + ":").grid(row=i, column=0, sticky="e", padx=5, pady=2)
            value_label = ttk.Label(self.machine_frame, text="N/A")
            value_label.grid(row=i, column=1, sticky="w", padx=5, pady=2)
            self.machine_params[key] = value_label

    def create_beaker_section(self):
        self.beaker_frame = ttk.LabelFrame(self.info_frame, text="Beaker Information")
        self.beaker_frame.grid(row=0, column=1, padx=5, pady=5, sticky="nsew")
        self.beaker_params = {}
        params = ["Current Temp", "RPM", "Dip Duration"]
        ttk.Label(self.beaker_frame, text="Beaker:").grid(row=0, column=0, columnspan=2)
        for j in range(6):
            ttk.Label(self.beaker_frame, text=f"{j+1}").grid(row=0, column=j+1)
        for i, param in enumerate(params):
            ttk.Label(self.beaker_frame, text=param + ":").grid(row=i+1, column=0, sticky="e", padx=5, pady=2)
            labels = []
            for j in range(6):
                label = ttk.Label(self.beaker_frame, text="N/A")
                label.grid(row=i+1, column=j+1, padx=2, pady=2)
                labels.append(label)
            self.beaker_params[param] = labels

    def create_data_frame(self):
        self.data_frame = ttk.LabelFrame(self.master, text="Received Data")
        self.data_frame.pack(fill=tk.X, padx=10, pady=10)
        self.data_text = tk.Text(self.data_frame, height=3, wrap=tk.NONE)
        self.data_text.pack(fill=tk.X, expand=True)
        self.data_text.config(state=tk.DISABLED)
        self.expand_button = ttk.Button(self.data_frame, text="Expand", command=self.toggle_data_frame)
        self.expand_button.pack(side=tk.RIGHT, padx=5, pady=5)
        self.data_frame_expanded = False

    def toggle_data_frame(self):
        if self.data_frame_expanded:
            self.data_text.config(height=3)
            self.expand_button.config(text="Expand")
        else:
            self.data_text.config(height=10)
            self.expand_button.config(text="Collapse")
        self.data_frame_expanded = not self.data_frame_expanded

    def setup_websocket(self):
        threading.Thread(target=self.start_websocket, daemon=True).start()

    async def connect_websocket(self):
        while True:
            try:
                uri = f"ws://DipMachine.local/ws"
                self.websocket = await websockets.connect(uri)
                self.update_connection_status("Connected")
                self.enable_choices()
                await self.receive_messages()
            except Exception as e:
                self.update_connection_status(f"Connection error: {str(e)}")
                await asyncio.sleep(5)

    async def receive_messages(self):
        try:
            while True:
                message = await self.websocket.recv()
                self.process_message(message)
        except websockets.ConnectionClosed:
            self.update_connection_status("Disconnected")
            self.update_server_state("Unknown")
            self.disable_choices()

    def process_message(self, message):
        try:
            data = json.loads(message)
            if 'state' in data:
                self.update_server_state(data['state'])
                self.update_machine_info(data)
                self.update_beaker_info(data)
            self.update_data_text(message)
        except json.JSONDecodeError:
            self.update_data_text(message)

    def update_machine_info(self, data):
        for key, label in self.machine_params.items():
            if key in data:
                label.config(text=str(data[key]))
        if "activeBeakers" in data:
            self.activeBeakers_spinbox.set(data["activeBeakers"])
        if "setCycles" in data:
            self.setCycles_spinbox.set(data["setCycles"])

    def update_beaker_info(self, data):
        beaker_data = {
            "Current Temp": data.get("currentTemp", []),
            "RPM": data.get("setDipRPM", []),  # Changed from "currentRPM" to "setDipRPM"
            "Dip Duration": data.get("setDipDuration", [])  # Changed from "timeLeftBeaker" to "setDipDuration"
        }
        for param, values in beaker_data.items():
            for i, value in enumerate(values):
                if i < len(self.beaker_params[param]):
                    self.beaker_params[param][i].config(text=str(value))
        if "setDipTemperature" in data:
            for i, value in enumerate(data["setDipTemperature"]):
                self.setDipTemperature_spinboxes[i].set(value)
        if "setDipDuration" in data:
            for i, value in enumerate(data["setDipDuration"]):
                self.setDipDuration_spinboxes[i].set(value)
        if "setDipRPM" in data:
            for i, value in enumerate(data["setDipRPM"]):
                self.setDipRPM_spinboxes[i].set(value)

    def update_data_text(self, message):
        self.data_text.config(state=tk.NORMAL)
        self.data_text.delete(1.0, tk.END)
        self.data_text.insert(tk.END, message)
        self.data_text.config(state=tk.DISABLED)

    def update_connection_status(self, status):
        self.master.after(0, lambda: self.connection_status_label.config(
            text=f"Connection: {status}",
            fg="green" if status == "Connected" else "red"
        ))

    def update_server_state(self, state):
        state_colors = {"IDLE": "blue", "HALTED": "orange", "WORKING": "green", "DONE": "purple", "POWERLOSS" : "red"}
        self.master.after(0, lambda: self.server_state_label.config(
            text=f"Server State: {state}",
            fg=state_colors.get(state, "black")
        ))
        self.update_ui_state(state)

    def update_ui_state(self, state):
        if state == "WORKING":
            self.disable_inputs()
            self.enable_abort_only()
        else:
            self.enable_inputs()
            self.enable_choices()

    def enable_choices(self):
        self.choice_combobox['values'] = self.status
        self.choice_combobox.set(self.status[0])
        self.choice_combobox['state'] = 'readonly'
        self.submit_choice_button['state'] = 'normal'

    def disable_choices(self):
        self.choice_combobox['state'] = 'disabled'
        self.submit_choice_button['state'] = 'disabled'

    def enable_abort_only(self):
        self.choice_combobox['values'] = ["abort"]
        self.choice_combobox.set("abort")
        self.choice_combobox['state'] = 'readonly'
        self.submit_choice_button['state'] = 'normal'

    def enable_inputs(self):
        for widget in self.input_frame.winfo_children():
            if isinstance(widget, (ttk.Spinbox, ttk.Entry)):
                widget['state'] = 'normal'

    def disable_inputs(self):
        for widget in self.input_frame.winfo_children():
            if isinstance(widget, (ttk.Spinbox, ttk.Entry)):
                widget['state'] = 'readonly'

    def submit_choice(self):
        selected_status = self.choice_combobox.get()
        self.msg["status"] = selected_status
        self.msg["activeBeakers"] = int(self.activeBeakers_spinbox.get())
        self.msg["setCycles"] = int(self.setCycles_spinbox.get())
        self.msg["setDipTemperature"] = [int(spinbox.get()) for spinbox in self.setDipTemperature_spinboxes]
        self.msg["setDipDuration"] = [int(spinbox.get()) for spinbox in self.setDipDuration_spinboxes]
        self.msg["setDipRPM"] = [int(spinbox.get()) for spinbox in self.setDipRPM_spinboxes]
        message = json.dumps(self.msg)
        if self.websocket and self.websocket.open:
            asyncio.run_coroutine_threadsafe(self.websocket.send(message), self.loop)
            self.update_data_text(f"Sent: {message}")
        else:
            self.update_data_text("Error: Cannot send message: WebSocket is not connected.")

    def start_websocket(self):
        self.loop.run_until_complete(self.connect_websocket())

if __name__ == "__main__":
    root = tk.Tk()
    app = WebSocketUI(root)
    root.mainloop()