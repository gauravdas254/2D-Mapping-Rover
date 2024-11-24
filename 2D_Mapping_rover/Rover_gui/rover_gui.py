import tkinter as tk
from tkinter import messagebox
import socket
import matplotlib.pyplot as plt
import numpy as np
from threading import Thread
import time

class RoverApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Rover Control")

        self.start_button = tk.Button(root, text="Start", command=self.start_rover)
        self.start_button.pack()

        self.stop_button = tk.Button(root, text="Stop", command=self.stop_rover)
        self.stop_button.pack()

        self.save_button = tk.Button(root, text="Save Map", command=self.save_map)
        self.save_button.pack()

        self.canvas = tk.Canvas(root, width=400, height=400)
        self.canvas.pack()

        self.coords = []
        self.running = False
        self.sock = None
        self.server_ip = "192.168.147.159"  # Replace with your ESP32's IP address
        self.server_port = 80

    def start_rover(self):
        self.running = True
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(10)  # Set timeout for socket operations
            self.sock.connect((self.server_ip, self.server_port))
            self.sock.sendall(b"START\n")
            Thread(target=self.update_map).start()
        except socket.error as e:
            messagebox.showerror("Connection Error", f"Could not connect to the rover: {e}")
            self.running = False

    def stop_rover(self):
        self.running = False
        if self.sock:
            try:
                self.sock.sendall(b"STOP\n")
            except socket.error as e:
                messagebox.showerror("Connection Error", f"Could not send stop command: {e}")
        if self.sock:
            self.sock.close()
            self.sock = None

    def save_map(self):
        if self.coords:
            plt.figure()
            x, y = zip(*self.coords)
            plt.plot(x, y, marker='o', color='blue')  # Connect points with lines
            plt.title("2D Map")
            plt.xlabel("X (cm)")
            plt.ylabel("Y (cm)")
            plt.savefig("2D_map.png")
            messagebox.showinfo("Save Map", "Map saved as 2D_map.png")
        else:
            messagebox.showwarning("Save Map", "No map data to save.")


    def update_map(self):
        while self.running:
            try:
                data = self.sock.recv(1024).decode('utf-8').strip()
                if data.startswith("COORD"):
                    parts = data.split(",")
                    if len(parts) == 3:
                        _, x, y = parts
                        x, y = float(x), float(y)
                        self.coords.append((x, y))
                        self.plot_coords()
                    else:
                        print(f"Unexpected data format: {data}")
            except socket.timeout:
                continue
            except Exception as e:
                messagebox.showerror("Error", f"An error occurred while updating the map: {e}")
                self.running = False
                break

    def plot_coords(self):
        self.canvas.delete("all")
        if self.coords:
            x, y = zip(*self.coords)
            x = np.array(x)
            y = np.array(y)

        # Avoid division by zero in scaling
            if np.max(x) != np.min(x):
                x_scaled = (x - np.min(x)) / (np.max(x) - np.min(x)) * 400
            else:
                x_scaled = np.full_like(x, 200)  # Center on the canvas

            if np.max(y) != np.min(y):
                y_scaled = (y - np.min(y)) / (np.max(y) - np.min(y)) * 400
            else:
                y_scaled = np.full_like(y, 200)  # Center on the canvas

        # Draw points and connect them with lines
            for i in range(len(x_scaled)):
                if i > 0:
                # Draw a line from the previous point to the current point
                    self.canvas.create_line(x_scaled[i-1], y_scaled[i-1], x_scaled[i], y_scaled[i], fill="blue", width=2)
            # Draw the current point
                self.canvas.create_oval(x_scaled[i] - 2, y_scaled[i] - 2, x_scaled[i] + 2, y_scaled[i] + 2, fill="red")


if __name__ == "__main__":
    root = tk.Tk()
    app = RoverApp(root)
    root.mainloop()
