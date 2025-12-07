import serial
import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets

# ----------------------------------------------------
# 🔧 CONFIGURATION
# ----------------------------------------------------
PORT = "COM7"          # change if needed
BAUD = 115200
FRAME_SAMPLES = 1000    # must match PSoC

# Effective sample rate after decimation:
# SAMPLE_RATE_HZ = (ADC actual SPS) / DECIM_FACTOR
SAMPLE_RATE_HZ = 30000.0   # update if your effective Fs is different

TIME_WINDOW_S = 0.005      # 5 ms on screen
N_PLOT = int(TIME_WINDOW_S * SAMPLE_RATE_HZ)

# ADC ideal full-scale voltage (your input range)
FULL_SCALE_V = 5.0

# Calibration (set to 1.0 for now; adjust later if needed)
CAL_GAIN = 1.0

# ----------------------------------------------------
# 🔌 SERIAL INITIALIZATION
# ----------------------------------------------------
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
    print(f"✅ Serial port {PORT} opened successfully at {BAUD} baud.")
except serial.SerialException as e:
    print(f"❌ Error opening serial port {PORT}: {e}")
    # Exit if serial port cannot be opened
    exit()

# ----------------------------------------------------
# 📈 APP SETUP (PYQTGRAPH)
# ----------------------------------------------------
app = QtWidgets.QApplication([])
pg.setConfigOptions(antialias=True)

win = pg.GraphicsLayoutWidget(title="PSoC Oscilloscope")
win.resize(900, 500)

plot = win.addPlot()
plot.setLabel('left', 'Voltage', units='V')
plot.setLabel('bottom', 'Time', units='ms')

# Set plot ranges
plot.setYRange(0, FULL_SCALE_V * CAL_GAIN)
plot.setXRange(0, TIME_WINDOW_S * 1000.0)

curve = plot.plot(pen='y')   # yellow trace

win.show()

# Time axis data for the plot (in ms)
t = np.arange(N_PLOT) / SAMPLE_RATE_HZ * 1000.0

# ----------------------------------------------------
# 💡 CONVERSION FUNCTION
# ----------------------------------------------------
def adc_to_volts(arr):
    """
    Convert 8-bit ADC values (0..255) to calibrated volts.
    The conversion is: (ADC_Value / 255) * FULL_SCALE_V * CAL_GAIN
    """
    return (arr.astype(np.float32) / 255.0) * FULL_SCALE_V * CAL_GAIN


# ----------------------------------------------------
# 📡 FRAME READER
# ----------------------------------------------------
def read_frame():
    """
    Read one frame, synchronized by a start byte (0xAA).
    The expected format is: 0xAA | length_byte (n) | data_bytes[n].
    """
    # Sync on 0xAA start byte
    while True:
        b = ser.read(1)
        if not b:
            return None  # Timeout
        if b[0] == 0xAA:
            break

    # Read length byte
    length = ser.read(1)
    if not length:
        return None

    n = length[0]  # The number of data bytes to read
    data = ser.read(n)
    
    # Check if the expected number of bytes was received
    if len(data) != n:
        print(f"Warning: Expected {n} bytes, got {len(data)}. Frame dropped.")
        return None

    # Convert the received data bytes into a NumPy array of 8-bit unsigned integers
    return np.frombuffer(data, dtype=np.uint8)


# ----------------------------------------------------
# ⚡ SOFTWARE TRIGGER IMPLEMENTATION
# ----------------------------------------------------
def trigger_align(frame, n_out):
    """
    Oscilloscope-style trigger on rising edge (half-way point).
    Returns n_out samples in volts, aligned to the trigger point.
    """
    vals_adc = frame.astype(np.float32)
    vals_v = adc_to_volts(frame)

    vmin_adc, vmax_adc = vals_adc.min(), vals_adc.max()
    p2p_adc = vmax_adc - vmin_adc

    # 1. FLAT SIGNAL CHECK: If signal is almost flat, return the first window
    if p2p_adc < 3:
        return vals_v[:n_out]

    # 2. TRIGGER LOGIC: Find crossings at the midpoint (50% level)
    thr = vmin_adc + p2p_adc / 2.0
    above = vals_adc > thr
    # Rising edge detection: (~above[:-1]) & (above[1:])
    crossings = np.where((~above[:-1]) & (above[1:]))[0]

    if len(crossings) == 0:
        return vals_v[:n_out] # No valid rising edge, return the first window

    # 3. ALIGNMENT: Start the plot window at the first rising edge
    start = crossings[0]
    end = start + n_out

    if end <= len(vals_v):
        # Full window available within the frame
        return vals_v[start:end]
    else:
        # Not enough data left for a full window; pad the end with the last value
        out = np.zeros(n_out, dtype=np.float32)
        sl = len(vals_v) - start
        out[:sl] = vals_v[start:]
        out[sl:] = vals_v[-1] # Simple hold-off padding
        return out

# ----------------------------------------------------
# 📐 ESTIMATION OF FREQUENCY AND AMPLITUDE
# ----------------------------------------------------
def estimate_freq_amp(frame):
    """
    Estimate frequency (from zero-crossings) and Vpp (from min/max)
    using the full frame data.
    """
    vals_adc = frame.astype(np.float32)

    vmin_adc, vmax_adc = vals_adc.min(), vals_adc.max()
    p2p_adc = vmax_adc - vmin_adc
    
    # Calculate Vpp (independent of frequency calculation)
    amp_vpp = (p2p_adc / 255.0) * FULL_SCALE_V * CAL_GAIN

    # Check for flat signal
    if p2p_adc < 3:
        return 0.0, 0.0 # Return 0 Hz if flat

    # Midpoint for frequency calculation
    thr = vmin_adc + p2p_adc / 2.0
    above = vals_adc > thr
    crossings = np.where((~above[:-1]) & (above[1:]))[0] # Rising edges

    if len(crossings) < 2:
        # Need at least two rising edges for a period calculation
        return 0.0, amp_vpp

    # Calculate periods and mean period
    periods = np.diff(crossings).astype(np.float32)
    mean_period = periods.mean() # Period in samples
    
    # Freq = (Samples per second) / (Samples per period)
    freq = SAMPLE_RATE_HZ / mean_period

    return freq, amp_vpp


# ----------------------------------------------------
# 🔄 MAIN UPDATE LOOP
# ----------------------------------------------------
def update():
    """
    Reads a new frame, processes it, and updates the plot and display.
    """
    frame = read_frame()
    if frame is None:
        return

    # 1. Trigger and Align
    aligned_v = trigger_align(frame, N_PLOT)
    curve.setData(t, aligned_v)

    # 2. Estimate Metrics (using the full frame)
    freq, amp = estimate_freq_amp(frame)

    # 3. Update Title
    plot.setTitle(f"Freq: {freq:7.1f} Hz    Amp: {amp:5.3f} Vpp")


# Setup the timer to call the update function periodically
timer = QtCore.QTimer()
timer.timeout.connect(update)
timer.start(1)  # call update every 1 ms

# ----------------------------------------------------
# 🚀 RUN APPLICATION
# ----------------------------------------------------
print("Starting PyQtGraph application...")
QtWidgets.QApplication.instance().exec_()
print("Application closed.")

# Close the serial port when the application exits
if ser.is_open:
    ser.close()
    print("Serial port closed.")