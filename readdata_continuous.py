import serial
import struct
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time
import numpy as np
from collections import deque

# ------------------ USER CONFIG ------------------
SERIAL_PORT = 'COM11'    # Change to your port (e.g., 'COM3' or '/dev/ttyUSB0')
BAUDRATE = 8000000       # Must match STM32 side
DISPLAY_SAMPLES = 100000 # Number of samples to keep visible on the plot
BYTES_PER_SAMPLE = 7
SCALE_FS2G = 0.061      # mg/LSB, for ±2g full-scale
UPDATE_INTERVAL = 50    # Animation update interval in milliseconds
# --------------------------------------------------

class SerialPlotter:
    def __init__(self, port, baudrate, display_samples):
        self.port = port
        self.baudrate = baudrate
        self.display_samples = display_samples
        self.serial_conn = None
        
        # Data buffers using deque for efficient append/pop operations
        self.x_data = deque(maxlen=display_samples)
        self.y_data = deque(maxlen=display_samples)
        self.z_data = deque(maxlen=display_samples)
        self.time_indices = deque(maxlen=display_samples)
        
        self.buffer = bytearray()
        self.sample_count = 0
        self.running = True
        self.last_data_time = time.time()  # Track when we last received data
        
        # Setup plot
        self.fig, self.ax = plt.subplots(figsize=(12, 8))
        self.line_x, = self.ax.plot([], [], 'r-', label='X [mg]', alpha=0.8)
        self.line_y, = self.ax.plot([], [], 'g-', label='Y [mg]', alpha=0.8)
        self.line_z, = self.ax.plot([], [], 'b-', label='Z [mg]', alpha=0.8)
        
        self.ax.set_title(f'Live IIS3DWB Acceleration Data (Showing last {display_samples:,} samples)')
        self.ax.set_xlabel('Sample Number')
        self.ax.set_ylabel('Acceleration [mg]')
        self.ax.legend()
        self.ax.grid(True, alpha=0.3)
        
        # Add sample counter text
        self.sample_text = self.ax.text(0.02, 0.95, '', transform=self.ax.transAxes, 
                                       bbox=dict(boxstyle="round,pad=0.3", facecolor="yellow", alpha=0.7))
    
    def connect_serial(self):
        """Connect to serial port"""
        try:
            self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=0.1)
            print(f"Connected to {self.port} at {self.baudrate} baud")
            return True
        except Exception as e:
            print(f"Failed to connect to serial port: {e}")
            return False
    
    def parse_new_samples(self):
        """Parse any complete samples from the buffer"""
        new_samples = 0
        while len(self.buffer) >= BYTES_PER_SAMPLE:
            # Parse raw X, Y, Z as int16 (little endian)
            x = struct.unpack_from('<h', self.buffer, 1)[0]
            y = struct.unpack_from('<h', self.buffer, 3)[0]
            z = struct.unpack_from('<h', self.buffer, 5)[0]
            
            # Convert to mg and add to buffers
            self.x_data.append(x * SCALE_FS2G)
            self.y_data.append(y * SCALE_FS2G)
            self.z_data.append(z * SCALE_FS2G)
            self.time_indices.append(self.sample_count)
            
            # Remove processed bytes
            del self.buffer[:BYTES_PER_SAMPLE]
            self.sample_count += 1
            new_samples += 1
        
        return new_samples
    
    def read_serial_data(self):
        """Read data from serial port"""
        if self.serial_conn and self.serial_conn.in_waiting > 0:
            try:
                data = self.serial_conn.read(self.serial_conn.in_waiting)
                if data:
                    self.buffer.extend(data)
                    self.last_data_time = time.time()  # Update last data time
                    return True
            except Exception as e:
                print(f"Serial read error: {e}")
        return False
    
    def animate(self, frame):
        """Animation function called by matplotlib"""
        if not self.running:
            return self.line_x, self.line_y, self.line_z, self.sample_text
        
        # Check if data has stopped coming (optional timeout)
        if time.time() - self.last_data_time > 30:  # 30 second timeout
            print("Warning: No data received for 30 seconds. Still listening...")
            self.last_data_time = time.time()  # Reset to avoid spam
        
        # Read new data from serial
        self.read_serial_data()
        
        # Parse any new samples
        new_samples = self.parse_new_samples()
        
        # Update plot if we have data
        if len(self.x_data) > 0:
            indices = list(self.time_indices)
            
            self.line_x.set_data(indices, list(self.x_data))
            self.line_y.set_data(indices, list(self.y_data))
            self.line_z.set_data(indices, list(self.z_data))
            
            # Auto-scale the plot
            if len(indices) > 1:
                self.ax.set_xlim(min(indices), max(indices))
                
                all_values = list(self.x_data) + list(self.y_data) + list(self.z_data)
                if all_values:
                    y_min, y_max = min(all_values), max(all_values)
                    y_range = y_max - y_min
                    if y_range > 0:
                        margin = y_range * 0.1
                        self.ax.set_ylim(y_min - margin, y_max + margin)
                    else:
                        self.ax.set_ylim(y_min - 1, y_max + 1)
        
        # Update sample counter (show total samples collected)
        self.sample_text.set_text(f'Total Samples: {self.sample_count:,} (Showing last {self.display_samples:,})')
        
        return self.line_x, self.line_y, self.line_z, self.sample_text
    
    def start_plotting(self):
        """Start the live plotting"""
        if not self.connect_serial():
            return
        
        print("Starting continuous live plot... Press Ctrl+C to stop")
        print(f"Displaying last {self.display_samples:,} samples on the plot")
        
        # Start animation
        self.ani = animation.FuncAnimation(
            self.fig, self.animate, interval=UPDATE_INTERVAL, 
            blit=True, cache_frame_data=False
        )
        
        try:
            plt.show()
        except KeyboardInterrupt:
            print("\nStopped by user")
        finally:
            if self.serial_conn:
                self.serial_conn.close()
                print("Serial connection closed")

def parse_samples(raw_bytes):
    xs, ys, zs = [], [], []
    for i in range(0, len(raw_bytes) - BYTES_PER_SAMPLE + 1, BYTES_PER_SAMPLE):
        # Parse raw X, Y, Z as int16 (little endian)
        x = struct.unpack_from('<h', raw_bytes, i+1)[0]
        y = struct.unpack_from('<h', raw_bytes, i+3)[0]
        z = struct.unpack_from('<h', raw_bytes, i+5)[0]
        # Convert to mg
        xs.append(x * SCALE_FS2G)
        ys.append(y * SCALE_FS2G)
        zs.append(z * SCALE_FS2G)
    print(f"Parsed {len(xs)} samples.")
    return xs, ys, zs

def main():
    """Main function to start the live serial plotter"""
    print(f"Starting continuous live serial plotter...")
    print(f"Port: {SERIAL_PORT}")
    print(f"Baudrate: {BAUDRATE}")
    print(f"Display buffer: {DISPLAY_SAMPLES:,} samples")
    
    plotter = SerialPlotter(SERIAL_PORT, BAUDRATE, DISPLAY_SAMPLES)
    plotter.start_plotting()

if __name__ == "__main__":
    main()
