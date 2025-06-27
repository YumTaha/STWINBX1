# IIS3DWB Vibration Sensor Data Logger

A Python toolkit for real-time data acquisition and visualization from the IIS3DWB 3-axis accelerometer via serial communication. This project provides two main scripts for different data collection scenarios: limited sample collection and continuous streaming.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Hardware Setup](#hardware-setup)
- [Usage](#usage)
  - [Limited Sample Collection](#limited-sample-collection)
  - [Continuous Data Streaming](#continuous-data-streaming)
- [Configuration](#configuration)
- [Data Format](#data-format)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)

## 🔍 Overview

This toolkit interfaces with STM32-based systems equipped with the **IIS3DWB** high-performance 3-axis accelerometer. The IIS3DWB is designed for vibration monitoring applications and can sample at rates up to 26.7 kHz, making it ideal for industrial condition monitoring and predictive maintenance applications.

### Key Components

1. **`readdata_1M.py`** - Collects a fixed number of samples (up to 1 million) with automatic stop
2. **`readdata_continuous.py`** - Provides continuous data streaming with a rolling display buffer

## ✨ Features

- **High-Speed Data Acquisition**: Supports baudrates up to 8 Mbps for real-time data streaming
- **Real-Time Visualization**: Live matplotlib plots with automatic scaling and multiple axis display
- **Flexible Sample Management**: Choose between fixed-count collection or continuous streaming
- **Robust Error Handling**: Built-in timeout detection and serial communication error recovery
- **Memory Efficient**: Uses deque data structures for optimal memory usage with large datasets
- **Cross-Platform**: Compatible with Windows, Linux, and macOS

## 📦 Requirements

### Python Dependencies

```
python >= 3.7
pyserial >= 3.5
matplotlib >= 3.5.0
numpy >= 1.20.0
```

### Hardware Requirements

- STM32 microcontroller with IIS3DWB sensor
- USB-to-Serial converter or direct USB connection
- Minimum 4GB RAM (recommended for 1M sample collection)

## 🚀 Installation

1. **Clone or download the repository**
   ```bash
   git clone <repository-url>
   cd iis3dwb-data-logger
   ```

2. **Install Python dependencies**
   ```bash
   pip install pyserial matplotlib numpy
   ```

3. **Connect your hardware** and identify the COM port

4. **Update configuration** in the Python files (see [Configuration](#configuration))

## 🔧 Hardware Setup

### STM32 Configuration

Your STM32 should be configured to:
- Initialize the IIS3DWB sensor with ±2g full-scale range
- Stream data at the configured baud rate (default: 8,000,000 bps)
- Send data in the expected 7-byte packet format

### Wiring

Ensure proper connections between:
- STM32 ↔ IIS3DWB (I2C/SPI)
- STM32 ↔ PC (USB/UART)

## 📖 Usage

### Limited Sample Collection

Use `readdata_1M.py` when you need to collect a specific number of samples and automatically stop.

```bash
python readdata_1M.py
```

**Features:**
- Collects up to 1,000,000 samples by default
- Automatically stops when target is reached
- Shows progress counter
- Ideal for batch processing and analysis

**Example Output:**
```
Starting live serial plotter (1M sample limit)...
Port: COM11
Baudrate: 8000000
Max samples: 1,000,000
Connected to COM11 at 8000000 baud
Starting live plot... Will stop after collecting 1,000,000 samples
Reached maximum samples (1,000,000). Stopping data collection.
Final count: 1,000,000 samples collected
```

### Continuous Data Streaming

Use `readdata_continuous.py` for ongoing monitoring and real-time analysis.

```bash
python readdata_continuous.py
```

**Features:**
- Continuous data collection
- Rolling display buffer (last 100,000 samples visible)
- Runs indefinitely until manually stopped
- Perfect for real-time monitoring

**Example Output:**
```
Starting continuous live serial plotter...
Port: COM11
Baudrate: 8000000
Display buffer: 100,000 samples
Connected to COM11 at 8000000 baud
Starting continuous live plot... Press Ctrl+C to stop
```

## ⚙️ Configuration

Both scripts can be configured by modifying the constants at the top of each file:

### Common Configuration Parameters

```python
# Serial Communication
SERIAL_PORT = 'COM11'        # Windows: 'COM3', Linux/Mac: '/dev/ttyUSB0'
BAUDRATE = 8000000           # Must match STM32 configuration

# Data Processing
BYTES_PER_SAMPLE = 7         # STM32 packet size
SCALE_FS2G = 0.061          # Conversion factor: mg/LSB for ±2g range

# Display Settings
UPDATE_INTERVAL = 50         # Plot refresh rate in milliseconds
```

### Script-Specific Parameters

**`readdata_1M.py`:**
```python
MAX_SAMPLES = 1000000        # Maximum samples before auto-stop
```

**`readdata_continuous.py`:**
```python
DISPLAY_SAMPLES = 100000     # Rolling buffer size for display
```

### Finding Your Serial Port

**Windows:**
- Check Device Manager → Ports (COM & LPT)
- Common: `COM3`, `COM4`, `COM11`

**Linux/Mac:**
```bash
ls /dev/tty*
# Look for /dev/ttyUSB0, /dev/ttyACM0, etc.
```

## 📊 Data Format

### Packet Structure

Each data packet from the STM32 consists of 7 bytes:

| Byte | Description |
|------|-------------|
| 0    | Start marker/timestamp |
| 1-2  | X-axis (int16, little-endian) |
| 3-4  | Y-axis (int16, little-endian) |
| 5-6  | Z-axis (int16, little-endian) |

### Data Conversion

Raw sensor data is converted to milligravity (mg) using:
```python
acceleration_mg = raw_value * SCALE_FS2G
```

Where `SCALE_FS2G = 0.061 mg/LSB` for ±2g full-scale range.

## 🎯 Plot Features

### Real-Time Display
- **Red line**: X-axis acceleration
- **Green line**: Y-axis acceleration  
- **Blue line**: Z-axis acceleration
- **Yellow box**: Sample counter and status

### Auto-Scaling
- Automatic X and Y axis scaling based on data range
- 10% margin added for better visualization
- Grid lines for easier reading

### Performance Optimization
- Efficient deque data structures
- Blitted animation for smooth updates
- Configurable update intervals

## 🔧 Troubleshooting

### Common Issues

**"Failed to connect to serial port"**
- Verify the COM port is correct
- Ensure no other applications are using the port
- Check cable connections
- Try different baud rates

**"No data received" warnings**
- Verify STM32 is transmitting data
- Check baud rate matching between STM32 and Python
- Ensure proper data packet format
- Verify power supply to the sensor

**Slow performance or choppy plots**
- Increase `UPDATE_INTERVAL` (less frequent updates)
- Reduce `DISPLAY_SAMPLES` for continuous mode
- Close other applications to free system resources
- Consider using a faster computer for high-speed data

**Memory issues with large datasets**
- Reduce `MAX_SAMPLES` in the 1M version
- Use continuous mode for long-term monitoring
- Monitor system RAM usage

### Performance Tips

1. **Optimize for your system**: Adjust `UPDATE_INTERVAL` based on your computer's performance
2. **Use appropriate buffer sizes**: Larger buffers need more RAM but show more history
3. **Close unnecessary applications**: Free up system resources for smooth operation
4. **USB connection**: Direct USB typically performs better than USB-to-Serial converters

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with your hardware setup
5. Submit a pull request

### Areas for Contribution

- Additional sensor support
- Data export functionality
- Advanced signal processing features
- GUI interface
- Configuration file support
- Unit tests

## 📝 License

This project is open source. Please check the license file for details.

## 🆘 Support

For issues and questions:
1. Check the [Troubleshooting](#troubleshooting) section
2. Search existing issues in the repository
3. Create a new issue with detailed information about your setup

---

**Happy Data Logging! 📈**
