# IIS3DWB Data Logger - Code Documentation

Technical documentation for understanding the internal workings of the IIS3DWB vibration sensor data logging scripts. This documentation explains the architecture, data flow, and implementation details of each component.

## 📋 Table of Contents

- [Architecture Overview](#architecture-overview)
- [File Structure](#file-structure)
- [Configuration Constants](#configuration-constants)
- [SerialPlotter Class](#serialplotter-class)
- [Data Processing Pipeline](#data-processing-pipeline)
- [Animation System](#animation-system)
- [Memory Management](#memory-management)
- [Error Handling](#error-handling)
- [Utility Functions](#utility-functions)

## 🏗️ Architecture Overview

Both scripts implement a real-time data acquisition and visualization system using an event-driven architecture:

```
Serial Port → Buffer → Parser → Data Structures → Matplotlib Animation → Display
     ↑                                                      ↓
   STM32                                               User Interface
```

### Key Design Patterns

1. **Producer-Consumer Pattern**: Serial reading produces data, animation consumes it
2. **Circular Buffer**: Using `deque` with `maxlen` for memory-efficient data storage
3. **State Machine**: `running` flag controls the application lifecycle
4. **Observer Pattern**: Matplotlib animation observes data changes

## 📁 File Structure

### `readdata_1M.py` - Fixed Sample Collection
- **Purpose**: Collects exactly `MAX_SAMPLES` samples then stops
- **Use Case**: Batch data collection for analysis
- **Memory**: Grows linearly up to maximum sample count

### `readdata_continuous.py` - Streaming Collection  
- **Purpose**: Continuous data collection with rolling window display
- **Use Case**: Real-time monitoring and live analysis
- **Memory**: Constant memory usage with circular buffer

## ⚙️ Configuration Constants

```python
# Serial Communication Parameters
SERIAL_PORT = 'COM11'        # Platform-specific serial port identifier
BAUDRATE = 8000000          # Bits per second - must match STM32 UART config

# Data Packet Structure
BYTES_PER_SAMPLE = 7        # Fixed packet size from STM32
SCALE_FS2G = 0.061         # Conversion factor: mg/LSB for ±2g full-scale

# Performance Tuning
UPDATE_INTERVAL = 50        # Matplotlib animation refresh rate (ms)
MAX_SAMPLES = 1000000       # (1M version) Maximum samples before auto-stop
DISPLAY_SAMPLES = 100000    # (Continuous) Rolling window size
```

### Design Rationale

- **High Baudrate**: 8 Mbps enables high-frequency sensor data streaming
- **Fixed Packet Size**: Simplifies parsing and ensures data integrity
- **Scale Factor**: Direct conversion from ADC counts to engineering units
- **Update Interval**: Balances responsiveness vs. CPU usage

## 🏭 SerialPlotter Class

Main class that encapsulates all functionality for serial data acquisition and visualization.

### Constructor (`__init__`)

```python
def __init__(self, port, baudrate, max_samples/display_samples):
```

**Purpose**: Initialize all components and data structures

**Key Operations**:
1. **Data Structure Setup**: Creates four `deque` objects for X, Y, Z data and time indices
2. **Plot Initialization**: Configures matplotlib figure, axes, and line objects
3. **State Variables**: Sets up counters, flags, and timing variables
4. **Serial Configuration**: Stores connection parameters for later use

**Memory Allocation**:
- `deque(maxlen=N)`: Pre-allocates circular buffer of size N
- Matplotlib objects: Figure, axis, and three line objects
- Bytearray buffer: Dynamic size based on incoming serial data

### Serial Connection (`connect_serial`)

```python
def connect_serial(self) -> bool:
```

**Purpose**: Establish serial communication with STM32

**Implementation Details**:
- Uses `pyserial` library with specified timeout (0.1s)
- Non-blocking reads to prevent UI freezing
- Returns boolean for connection status checking
- Exception handling for port access issues

**Error Scenarios**:
- Port already in use by another application
- Invalid port name or hardware disconnection
- Insufficient permissions on Linux/Mac systems

### Data Parsing (`parse_new_samples`)

```python
def parse_new_samples(self) -> int:
```

**Purpose**: Extract complete 7-byte packets from the serial buffer

**Algorithm**:
1. **Packet Detection**: Check if buffer contains at least `BYTES_PER_SAMPLE` bytes
2. **Binary Unpacking**: Use `struct.unpack_from('<h', buffer, offset)` for little-endian int16
3. **Unit Conversion**: Multiply raw values by `SCALE_FS2G` for mg units
4. **Buffer Management**: Remove processed bytes using `del buffer[:7]`
5. **Data Storage**: Append to respective deque structures

**Byte Layout**:
```
[0] - Header/timestamp (ignored in current implementation)
[1-2] - X-axis acceleration (int16, little-endian)
[3-4] - Y-axis acceleration (int16, little-endian)  
[5-6] - Z-axis acceleration (int16, little-endian)
```

**Performance Considerations**:
- `struct.unpack_from()` is faster than slicing + `struct.unpack()`
- `del buffer[:n]` efficiently removes processed data
- Loop processes all available complete packets in one call

### Serial Data Reading (`read_serial_data`)

```python
def read_serial_data(self) -> bool:
```

**Purpose**: Non-blocking read from serial port

**Implementation**:
1. **Availability Check**: `serial_conn.in_waiting` returns bytes available
2. **Bulk Read**: Read all available data at once for efficiency
3. **Buffer Extension**: Append new bytes to existing buffer using `extend()`
4. **Timestamp Update**: Track last successful read for timeout detection

**Non-blocking Design**:
- Only reads when data is available (`in_waiting > 0`)
- Returns immediately if no data present
- Prevents animation from freezing during serial delays

### Animation Loop (`animate`)

```python
def animate(self, frame) -> tuple:
```

**Purpose**: Core animation callback function called by matplotlib

**Execution Flow**:
1. **State Check**: Return early if `running = False`
2. **Timeout Detection**: Check for data starvation
3. **Data Acquisition**: Call `read_serial_data()`
4. **Data Processing**: Call `parse_new_samples()`
5. **Plot Update**: Update line data and axis scaling
6. **UI Update**: Refresh sample counter display

**Return Value**: Tuple of plot objects for blitting optimization

**Auto-scaling Algorithm**:
```python
# Combine all axis data for range calculation
all_values = list(x_data) + list(y_data) + list(z_data)
y_min, y_max = min(all_values), max(all_values)
margin = (y_max - y_min) * 0.1  # 10% margin
ax.set_ylim(y_min - margin, y_max + margin)
```

### Application Lifecycle (`start_plotting`)

```python
def start_plotting(self):
```

**Purpose**: Main application entry point and lifecycle management

**Sequence**:
1. **Connection**: Establish serial communication
2. **Animation Setup**: Create `FuncAnimation` object with blitting enabled
3. **Event Loop**: Enter matplotlib's blocking event loop
4. **Cleanup**: Handle graceful shutdown on exit or Ctrl+C

**Animation Configuration**:
- `interval=UPDATE_INTERVAL`: Refresh rate in milliseconds
- `blit=True`: Enable blitting for better performance
- `cache_frame_data=False`: Prevent memory accumulation

## 🔄 Data Processing Pipeline

### Stage 1: Serial Reception
- **Input**: Raw bytes from STM32 UART
- **Buffer**: `bytearray()` accumulates incoming data
- **Rate**: Up to 8 Mbps continuous stream

### Stage 2: Packet Parsing
- **Input**: Buffered byte stream
- **Process**: Extract 7-byte packets using `struct.unpack_from()`
- **Output**: Raw int16 values for X, Y, Z axes

### Stage 3: Unit Conversion
- **Input**: Raw ADC counts (int16)
- **Process**: Multiply by `SCALE_FS2G` constant
- **Output**: Acceleration values in milligravity (mg)

### Stage 4: Data Storage
- **Structure**: Three separate `deque` objects per axis
- **Indexing**: Monotonic sample counter for time reference
- **Memory**: Circular buffer with automatic old data eviction

### Stage 5: Visualization
- **Input**: Current data in deque structures
- **Process**: Convert to lists for matplotlib line objects
- **Output**: Real-time plot updates

## 🎬 Animation System

### Matplotlib Integration

The animation system uses `matplotlib.animation.FuncAnimation`:

```python
self.ani = animation.FuncAnimation(
    self.fig, self.animate, 
    interval=UPDATE_INTERVAL,
    blit=True, 
    cache_frame_data=False
)
```

### Blitting Optimization

**Concept**: Only redraw changed parts of the plot
**Implementation**: Return tuple of modified artists from `animate()`
**Benefit**: Significant performance improvement for high-refresh-rate plots

### Line Object Management

Three separate line objects for each axis:
```python
self.line_x, = self.ax.plot([], [], 'r-', label='X [mg]', alpha=0.8)
self.line_y, = self.ax.plot([], [], 'g-', label='Y [mg]', alpha=0.8)  
self.line_z, = self.ax.plot([], [], 'b-', label='Z [mg]', alpha=0.8)
```

**Color Coding**: Red=X, Green=Y, Blue=Z (standard convention)
**Alpha**: 0.8 transparency for overlapping line visibility

## 💾 Memory Management

### Deque Data Structures

```python
from collections import deque

self.x_data = deque(maxlen=max_samples)
self.y_data = deque(maxlen=max_samples)
self.z_data = deque(maxlen=max_samples)
self.time_indices = deque(maxlen=max_samples)
```

**Advantages**:
- **O(1)** append and pop operations
- **Automatic memory management**: Old data automatically evicted when full
- **Memory bounded**: Maximum memory usage is predictable
- **Cache friendly**: Contiguous memory layout

### Buffer Management

**Serial Buffer**: `bytearray()` for incoming serial data
- Grows dynamically as data arrives
- Shrinks as packets are processed
- Typical size: 7-70 bytes (1-10 packets)

**Plot Conversion**: Temporary list creation for matplotlib
```python
indices = list(self.time_indices)  # Convert deque to list
self.line_x.set_data(indices, list(self.x_data))
```

### Memory Differences Between Scripts

**`readdata_1M.py`**:
- Linear memory growth up to 1M samples
- Peak usage: ~32MB for 1M samples (4 × 8 bytes × 1M)
- Fixed endpoint with cleanup

**`readdata_continuous.py`**:
- Constant memory usage after initial fill
- Steady state: ~3.2MB for 100K sample window
- Runs indefinitely with bounded memory

## ⚠️ Error Handling

### Serial Communication Errors

```python
try:
    data = self.serial_conn.read(self.serial_conn.in_waiting)
    if data:
        self.buffer.extend(data)
        self.last_data_time = time.time()
        return True
except Exception as e:
    print(f"Serial read error: {e}")
```

**Error Types**:
- **Device disconnection**: USB cable unplugged
- **Port busy**: Another application using the port
- **Timeout**: No response from STM32
- **Data corruption**: Invalid bytes received

### Timeout Detection

```python
if time.time() - self.last_data_time > timeout_seconds:
    print("Warning: No data received for X seconds")
```

**Timeout Values**:
- `readdata_1M.py`: 10 seconds (more aggressive)
- `readdata_continuous.py`: 30 seconds (more tolerant)

**Recovery Strategy**: Continue listening, don't exit immediately

### Graceful Shutdown

```python
try:
    plt.show()
except KeyboardInterrupt:
    print("\nStopped by user")
finally:
    if self.serial_conn:
        self.serial_conn.close()
        print("Serial connection closed")
```

**Cleanup Operations**:
1. Close serial port connection
2. Print final statistics
3. Release matplotlib resources

## 🛠️ Utility Functions

### Legacy Parser (`parse_samples`)

```python
def parse_samples(raw_bytes) -> tuple[list, list, list]:
```

**Purpose**: Batch processing of raw byte buffers (compatibility function)
**Usage**: Not used in current real-time implementation
**Returns**: Three lists containing X, Y, Z acceleration data

**Algorithm**:
```python
for i in range(0, len(raw_bytes) - BYTES_PER_SAMPLE + 1, BYTES_PER_SAMPLE):
    x = struct.unpack_from('<h', raw_bytes, i+1)[0]
    y = struct.unpack_from('<h', raw_bytes, i+3)[0] 
    z = struct.unpack_from('<h', raw_bytes, i+5)[0]
    xs.append(x * SCALE_FS2G)
    ys.append(y * SCALE_FS2G)
    zs.append(z * SCALE_FS2G)
```

### Main Entry Point (`main`)

```python
def main():
```

**Purpose**: Application bootstrap and configuration display
**Operations**:
1. Print configuration summary
2. Create `SerialPlotter` instance  
3. Start the plotting system

**Configuration Summary**:
- Serial port and baud rate
- Sample limits or buffer sizes
- Helpful for debugging connection issues

---

## 🔍 Code Flow Summary

1. **Initialization**: Create data structures and plot objects
2. **Connection**: Establish serial communication with STM32
3. **Animation Loop**: 
   - Read available serial data
   - Parse complete packets
   - Update data structures
   - Refresh plot display
4. **Cleanup**: Close connections and display statistics

This architecture provides robust, real-time data acquisition with efficient memory usage and responsive user interface.
