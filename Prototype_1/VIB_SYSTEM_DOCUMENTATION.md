# Vibration I/O System - Modular Architecture Documentation

## Overview
This document explains the modular architecture implemented for the vibration sensor data acquisition and transmission system. The original monolithic `vib_io.c` file has been split into three specialized modules for better maintainability, testability, and code organization.

## Module Architecture

```
┌─────────────────┐
│     main.c      │ ← Application Layer
├─────────────────┤
│    vib_io.c     │ ← High-Level Coordinator
├─────────────────┤
│  vib_sensor.c   │ ← Sensor Management
│  vib_comm.c     │ ← Communication Layer
└─────────────────┘
```

---

## 1. VIB_SENSOR Module (`vib_sensor.c/h`)

### Purpose
Handles all sensor-related operations including SPI communication, sensor initialization, FIFO management, and double-buffered data acquisition.

### Key Data Structure: `vib_double_buffer_t`

```c
typedef struct {
    iis3dwb_fifo_out_raw_t *buffer_A;        // First half-buffer
    iis3dwb_fifo_out_raw_t *buffer_B;        // Second half-buffer
    iis3dwb_fifo_out_raw_t *active_buffer;   // Currently being filled
    iis3dwb_fifo_out_raw_t *dma_buffer;      // Currently being transmitted
    uint32_t active_index;                   // Current position in active buffer
    uint32_t buffer_size;                    // Size of each half-buffer
    volatile uint8_t dma_in_progress;        // Flag indicating DMA transmission active
} vib_double_buffer_t;
```

**Explanation:**
- **Double Buffering Design**: Uses two half-sized buffers (0.5 seconds each) instead of one full buffer
- **Active Buffer**: Currently being filled with incoming sensor data
- **DMA Buffer**: Currently being transmitted via UART DMA
- **Buffer Swapping**: When active buffer is full, it becomes the DMA buffer, and vice versa
- **Thread Safety**: `volatile` keyword ensures DMA flag is always read from memory

### Key Functions

#### `vib_sensor_read_fifo_data_double_buffer(uint32_t *num_samples)`

**Purpose**: Main data acquisition function implementing double-buffered continuous streaming.

**Detailed Functionality:**

```c
const iis3dwb_fifo_out_raw_t* vib_sensor_read_fifo_data_double_buffer(uint32_t *num_samples) {
    uint16_t samples_to_read = 0;
    
    // Initialize output parameters
    if (num_samples) *num_samples = 0;

    // 1. GET FIFO STATUS
    if (iis3dwb_fifo_status_get(&dev_ctx, &fifo_status) != 0) {
        Error_Handler();
        return NULL;
    }
```

**Step 1 - FIFO Status Check:**
- Queries the IIS3DWB sensor for current FIFO status
- Checks if FIFO threshold interrupt flag is set
- Returns NULL if sensor communication fails

```c
    // 2. READ AVAILABLE SAMPLES
    if (fifo_status.fifo_th) {
        samples_to_read = fifo_status.fifo_level;
        
        // Overflow protection
        if ((double_buffer.active_index + samples_to_read) > double_buffer.buffer_size) {
            samples_to_read = double_buffer.buffer_size - double_buffer.active_index;
        }
```

**Step 2 - Sample Calculation:**
- `fifo_status.fifo_th`: FIFO threshold interrupt flag (set when FIFO has ≥500 samples)
- `fifo_status.fifo_level`: Actual number of samples available in sensor FIFO
- **Overflow Protection**: Ensures we never write past the end of the active buffer

```c
        // Read samples into active buffer at current position
        if (iis3dwb_fifo_out_multi_raw_get(&dev_ctx, 
                                          &double_buffer.active_buffer[double_buffer.active_index], 
                                          samples_to_read) == 0) {
            double_buffer.active_index += samples_to_read;
        }
```

**Step 3 - Data Transfer:**
- `iis3dwb_fifo_out_multi_raw_get()`: Reads multiple samples from sensor FIFO in one SPI transaction
- Writes directly to active buffer at current index position
- Updates active_index to track how full the buffer is

```c
        // 3. BUFFER SWAP LOGIC
        if (double_buffer.active_index == double_buffer.buffer_size && !double_buffer.dma_in_progress) {
            // Swap buffers
            iis3dwb_fifo_out_raw_t *tmp = double_buffer.dma_buffer;
            double_buffer.dma_buffer = double_buffer.active_buffer;
            double_buffer.active_buffer = tmp;
            
            double_buffer.active_index = 0;
            double_buffer.dma_in_progress = 1;
            
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            
            if (num_samples) *num_samples = double_buffer.buffer_size;
            return double_buffer.dma_buffer;
        }
```

**Step 4 - Buffer Swapping:**
- **Condition**: Active buffer is full AND no DMA transmission in progress
- **Pointer Swap**: Active buffer becomes DMA buffer, DMA buffer becomes active buffer
- **Reset**: Active index reset to 0 for new buffer
- **Synchronization**: Set DMA flag to prevent multiple simultaneous transmissions
- **Visual Feedback**: Toggle LED1 to indicate buffer ready
- **Return**: Provides full buffer to communication layer

#### `vib_sensor_dma_transmission_complete()`

**Purpose**: Callback function to reset DMA flag when transmission completes.

```c
void vib_sensor_dma_transmission_complete(void) {
    double_buffer.dma_in_progress = 0; // Allow next buffer swap
}
```

**Functionality:**
- Called by communication module when DMA transmission finishes
- Resets the `dma_in_progress` flag
- Enables the next buffer swap to occur
- Critical for continuous operation

#### `vib_sensor_get_active_buffer_level()`

**Purpose**: Monitoring function for debugging and system status.

```c
uint32_t vib_sensor_get_active_buffer_level(void) {
    return double_buffer.active_index;
}
```

**Use Cases:**
- Debugging buffer fill rates
- System health monitoring
- Performance analysis
- Real-time status display

---

## 2. VIB_COMM Module (`vib_comm.c/h`)

### Purpose
Handles all UART DMA communication for transmitting sensor data. Manages large data transfers by chunking them into DMA-compatible sizes.

### Key Functions

#### `vib_comm_send_buffer_uart_dma(const iis3dwb_fifo_out_raw_t *buffer, uint32_t num_samples)`

**Purpose**: Initiates non-blocking DMA transmission of sensor data buffer.

**Detailed Functionality:**

```c
vib_comm_status_t vib_comm_send_buffer_uart_dma(const iis3dwb_fifo_out_raw_t *buffer, uint32_t num_samples)
{
    // 1. BUSY CHECK
    if (uart_dma_tx.busy) return VIB_COMM_BUSY;
```

**Step 1 - Collision Detection:**
- Prevents starting new transmission while previous one is active
- Returns `VIB_COMM_BUSY` if transmission already in progress
- Ensures data integrity

```c
    // 2. SETUP TRANSMISSION PARAMETERS
    uart_dma_tx.data_ptr = (uint8_t*)buffer;
    uart_dma_tx.bytes_remaining = num_samples * sizeof(iis3dwb_fifo_out_raw_t);
    uart_dma_tx.busy = 1;
```

**Step 2 - Parameter Setup:**
- `data_ptr`: Points to the data buffer to transmit
- `bytes_remaining`: Total bytes to send (samples × 6 bytes per sample)
- `busy`: Flag to indicate transmission in progress

```c
    // 3. CHUNKING FOR DMA LIMITATIONS
    uint16_t chunk = (uart_dma_tx.bytes_remaining > 0xFFFF) ? 0xFFFF : uart_dma_tx.bytes_remaining;
    
    if (HAL_UART_Transmit_DMA(&huart2, uart_dma_tx.data_ptr, chunk) != HAL_OK) {
        uart_dma_tx.busy = 0;
        return VIB_COMM_ERROR;
    }
    
    uart_dma_tx.bytes_remaining -= chunk;
    uart_dma_tx.data_ptr += chunk;
```

**Step 3 - DMA Chunking:**
- **DMA Limitation**: STM32 DMA can only transfer up to 65535 bytes at once
- **Chunking**: Splits large buffers into DMA-compatible chunks
- **State Tracking**: Updates pointers and remaining byte count
- **Error Handling**: Resets busy flag if DMA start fails

#### `vib_comm_uart_dma_tx_complete_callback(UART_HandleTypeDef *huart)`

**Purpose**: Handles DMA transmission completion and manages multi-chunk transfers.

**Detailed Functionality:**

```c
void vib_comm_uart_dma_tx_complete_callback(UART_HandleTypeDef *huart)
{
    if (huart != &huart2) return;  // Ensure correct UART instance
```

**Step 1 - UART Validation:**
- Ensures callback is for the correct UART instance
- Prevents interference from other UART operations

```c
    if (uart_dma_tx.bytes_remaining > 0) {
        // More chunks to send
        uint16_t chunk = (uart_dma_tx.bytes_remaining > 0xFFFF) ? 0xFFFF : uart_dma_tx.bytes_remaining;
        if (HAL_UART_Transmit_DMA(&huart2, uart_dma_tx.data_ptr, chunk) != HAL_OK) {
            uart_dma_tx.busy = 0;
            Error_Handler();
        }
        uart_dma_tx.bytes_remaining -= chunk;
        uart_dma_tx.data_ptr += chunk;
```

**Step 2 - Continuation Logic:**
- **Multi-Chunk Handling**: If more data remains, start next DMA transfer
- **Automatic Progression**: Advances data pointer and decrements remaining bytes
- **Error Recovery**: Resets state if DMA restart fails

```c
    } else {
        // Transmission complete
        uart_dma_tx.busy = 0;
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
        
        // Notify sensor module
        vib_sensor_dma_transmission_complete();
    }
```

**Step 3 - Completion Handling:**
- **State Reset**: Clear busy flag to allow next transmission
- **Visual Feedback**: Toggle LED2 to indicate transmission complete
- **Module Notification**: Inform sensor module that DMA buffer is free

#### `vib_comm_is_busy()`

**Purpose**: Query function to check transmission status.

```c
uint8_t vib_comm_is_busy(void) {
    return uart_dma_tx.busy;
}
```

#### `vib_comm_get_bytes_remaining()`

**Purpose**: Debug/monitoring function to check transmission progress.

```c
uint32_t vib_comm_get_bytes_remaining(void) {
    return uart_dma_tx.bytes_remaining;
}
```

---

## 3. VIB_IO Module (`vib_io.c/h`)

### Purpose
High-level coordinator that provides a clean interface to the application layer. Orchestrates interaction between sensor and communication modules.

### Key Function

#### `vib_io_process()`

**Purpose**: Main processing function that coordinates sensor data acquisition and transmission.

**Detailed Functionality:**

```c
void vib_io_process(void) {
    uint32_t num_samples = 0;
    const iis3dwb_fifo_out_raw_t *buffer = vib_sensor_read_fifo_data_double_buffer(&num_samples);
```

**Step 1 - Data Acquisition:**
- Calls sensor module to check for available data
- Uses double-buffering function for continuous streaming
- Receives buffer pointer and sample count

```c
    if (buffer != NULL && num_samples > 0) {
        // Buffer is ready for transmission
        vib_comm_status_t status = vib_comm_send_buffer_uart_dma(buffer, num_samples);
        if (status != VIB_COMM_OK) {
            // If transmission fails, notify sensor to reset DMA flag
            vib_sensor_dma_transmission_complete();
        }
    }
```

**Step 2 - Transmission Coordination:**
- **Immediate Transmission**: No busy check needed (handled in comm module)
- **Error Recovery**: If transmission fails, notify sensor to reset DMA flag
- **Automatic Flow Control**: System naturally paces itself based on transmission speed

---

## System Benefits

### 1. **Continuous Streaming**
- **Before**: Batch processing with gaps between transmissions
- **After**: Continuous data flow with double buffering

### 2. **Reduced Latency**
- **Before**: Up to 1 second delay
- **After**: Maximum 0.5 second delay

### 3. **Better Resource Utilization**
- **Parallel Operations**: Data acquisition and transmission occur simultaneously
- **No CPU Blocking**: All operations are interrupt/DMA driven

### 4. **Improved Maintainability**
- **Modular Design**: Each module has a single responsibility
- **Clean Interfaces**: Well-defined APIs between modules
- **Easy Testing**: Individual modules can be tested in isolation

### 5. **Scalability**
- **Easy Extensions**: New communication methods can be added
- **Flexible Buffering**: Buffer sizes can be adjusted per module
- **Multiple Sensors**: Architecture supports multiple sensor instances

## Data Flow Summary

```
Sensor FIFO → vib_sensor → Double Buffer → vib_comm → UART DMA → Output
     ↑            ↑            ↑           ↑         ↑         ↑
   26.7kHz    Buffer Swap   0.5s chunks  Chunking  65KB max  Continuous
```

This modular architecture provides a robust, maintainable, and efficient solution for high-speed vibration data acquisition and transmission.
