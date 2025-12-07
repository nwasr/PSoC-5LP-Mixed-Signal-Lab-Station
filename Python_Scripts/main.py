import serial
import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets

# ---------------- Serial config ----------------
PORT = "COM7"     
BAUD = 115200
FRAME_SAMPLES = 252
SAMPLE_RATE_HZ = 30000.0

TIME_WINDOW_S = 0.005
N_PLOT = int(TIME_WINDOW_S * SAMPLE_RATE_HZ)

FULL_SCALE_V = 5.0
CAL_GAIN = 1.0


class ScopeFuncGenRC(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        # ---- serial ----
        try:
            self.ser = serial.Serial(PORT, BAUD, timeout=0)
            print(f"Opened {PORT} at {BAUD}")
        except Exception as e:
            self.ser = None
            print(f"Serial open failed: {e}")

        self.frame_state = "idle"
        self.pending_len = 0
        self.pending_data = bytearray()
        self.line_buf = ""

        # last raw frame (for save/export)
        self.last_frame = None

        # ---- plotting config (dark theme) ----
        pg.setConfigOptions(antialias=True)
        pg.setConfigOption('background', '#111111')
        pg.setConfigOption('foreground', '#DDDDDD')

        # ---- UI ----
        self.init_ui()
        self.apply_style()

        # ---- plotting ----
        self.t = np.arange(N_PLOT) / SAMPLE_RATE_HZ * 1000.0  # ms

        # ---- timer ----
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.poll_serial)
        self.timer.start(5)

    # ------------- UI -------------
    def init_ui(self):
        self.setWindowTitle("PSoC Lab Station")
        self.resize(1200, 650)

        main = QtWidgets.QHBoxLayout(self)
        main.setContentsMargins(10, 10, 10, 10)
        main.setSpacing(10)

        # ========= LEFT: Scope =========
        left_panel = QtWidgets.QVBoxLayout()
        left_panel.setSpacing(6)

        title_label = QtWidgets.QLabel("Oscilloscope")
        title_label.setObjectName("TitleLabel")
        left_panel.addWidget(title_label)

        self.plot = pg.PlotWidget()
        self.plot.setLabel('left', 'Voltage', units='V')
        self.plot.setLabel('bottom', 'Time', units='ms')
        self.plot.setYRange(0, FULL_SCALE_V * CAL_GAIN)
        self.plot.setXRange(0, TIME_WINDOW_S * 1000.0)
        self.curve = self.plot.plot(pen=pg.mkPen(width=2))
        left_panel.addWidget(self.plot, 1)

        main.addLayout(left_panel, 3)

        # ========= RIGHT: Controls =========
        right_panel = QtWidgets.QVBoxLayout()
        right_panel.setSpacing(8)

        header = QtWidgets.QLabel("Function Generator & Meter")
        header.setObjectName("HeaderLabel")
        right_panel.addWidget(header)

        # --------- Generator Card ----------
        gen_card = QtWidgets.QFrame()
        gen_card.setObjectName("Card")
        gen_layout = QtWidgets.QVBoxLayout(gen_card)
        gen_layout.setContentsMargins(10, 10, 10, 10)
        gen_layout.setSpacing(8)

        # Frequency
        freq_group = QtWidgets.QGroupBox("Frequency (Hz)")
        freq_group.setObjectName("Group")
        f_layout = QtWidgets.QVBoxLayout(freq_group)

        self.freq_label = QtWidgets.QLabel("1000 Hz")
        self.freq_label.setObjectName("ValueLabel")
        f_layout.addWidget(self.freq_label)

        self.freq_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.freq_slider.setRange(0, 3000)
        self.freq_slider.setValue(1000)
        self.freq_slider.valueChanged.connect(self.on_freq_change)
        f_layout.addWidget(self.freq_slider)

        ends = QtWidgets.QHBoxLayout()
        ends.addWidget(QtWidgets.QLabel("0"))
        ends.addStretch()
        ends.addWidget(QtWidgets.QLabel("3000"))
        f_layout.addLayout(ends)

        gen_layout.addWidget(freq_group)

        # Amplitude
        amp_group = QtWidgets.QGroupBox("Amplitude (%)")
        amp_group.setObjectName("Group")
        a_layout = QtWidgets.QVBoxLayout(amp_group)

        self.amp_label = QtWidgets.QLabel("100 %")
        self.amp_label.setObjectName("ValueLabel")
        a_layout.addWidget(self.amp_label)

        self.amp_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.amp_slider.setRange(0, 100)
        self.amp_slider.setValue(100)
        self.amp_slider.valueChanged.connect(self.on_amp_change)
        a_layout.addWidget(self.amp_slider)

        a_ends = QtWidgets.QHBoxLayout()
        a_ends.addWidget(QtWidgets.QLabel("0"))
        a_ends.addStretch()
        a_ends.addWidget(QtWidgets.QLabel("100"))
        a_layout.addLayout(a_ends)

        gen_layout.addWidget(amp_group)

        # Waveform
        wave_group = QtWidgets.QGroupBox("Waveform")
        wave_group.setObjectName("Group")
        w_layout = QtWidgets.QHBoxLayout(wave_group)
        self.wave_var = QtWidgets.QButtonGroup(self)

        self.rb_sine = QtWidgets.QRadioButton("Sine")
        self.rb_tri  = QtWidgets.QRadioButton("Triangle")
        self.rb_sqr  = QtWidgets.QRadioButton("Square")
        self.rb_sine.setChecked(True)

        self.wave_var.addButton(self.rb_sine, 0)
        self.wave_var.addButton(self.rb_tri, 1)
        self.wave_var.addButton(self.rb_sqr, 2)

        w_layout.addWidget(self.rb_sine)
        w_layout.addWidget(self.rb_tri)
        w_layout.addWidget(self.rb_sqr)

        gen_layout.addWidget(wave_group)

        # Start/Stop
        btn_row = QtWidgets.QHBoxLayout()
        self.btn_start = QtWidgets.QPushButton("Start / Apply")
        self.btn_stop  = QtWidgets.QPushButton("Stop")
        self.btn_start.clicked.connect(self.send_start)
        self.btn_stop.clicked.connect(self.send_stop)
        btn_row.addWidget(self.btn_start)
        btn_row.addWidget(self.btn_stop)
        gen_layout.addLayout(btn_row)

        right_panel.addWidget(gen_card)

        # --------- Meter Card ----------
        meter_card = QtWidgets.QFrame()
        meter_card.setObjectName("Card")
        meter_layout = QtWidgets.QVBoxLayout(meter_card)
        meter_layout.setContentsMargins(10, 10, 10, 10)
        meter_layout.setSpacing(8)

        # Resistance
        r_group = QtWidgets.QGroupBox("Resistance")
        r_group.setObjectName("Group")
        rg_layout = QtWidgets.QHBoxLayout(r_group)
        self.label_R = QtWidgets.QLabel("R (Ω): ---")
        self.label_R.setObjectName("BigValueLabel")
        self.btn_measR = QtWidgets.QPushButton("Measure R")
        self.btn_measR.clicked.connect(self.send_meas_r)
        rg_layout.addWidget(self.label_R)
        rg_layout.addStretch()
        rg_layout.addWidget(self.btn_measR)
        meter_layout.addWidget(r_group)

        # Capacitance
        c_group = QtWidgets.QGroupBox("Capacitance")
        c_group.setObjectName("Group")
        cg_layout = QtWidgets.QHBoxLayout(c_group)
        self.label_C = QtWidgets.QLabel("C (µF): ---")
        self.label_C.setObjectName("BigValueLabel")
        self.btn_measC = QtWidgets.QPushButton("Measure C")
        self.btn_measC.clicked.connect(self.send_meas_c)
        cg_layout.addWidget(self.label_C)
        cg_layout.addStretch()
        cg_layout.addWidget(self.btn_measC)
        meter_layout.addWidget(c_group)

        right_panel.addWidget(meter_card)

        # --------- Save + Status + Quit ----------
        bottom_card = QtWidgets.QFrame()
        bottom_card.setObjectName("Card")
        bottom_layout = QtWidgets.QVBoxLayout(bottom_card)
        bottom_layout.setContentsMargins(10, 10, 10, 10)
        bottom_layout.setSpacing(6)

        # Save + Quit in one row
        btn_bar = QtWidgets.QHBoxLayout()
        self.btn_save = QtWidgets.QPushButton("Save Waveform…")
        self.btn_quit = QtWidgets.QPushButton("Quit")
        self.btn_quit.setObjectName("QuitButton")
        self.btn_save.clicked.connect(self.save_waveform)
        self.btn_quit.clicked.connect(self.quit_app)
        btn_bar.addWidget(self.btn_save)
        btn_bar.addStretch()
        btn_bar.addWidget(self.btn_quit)
        bottom_layout.addLayout(btn_bar)

        self.status_label = QtWidgets.QLabel("Status: Ready")
        self.status_label.setObjectName("StatusLabel")
        bottom_layout.addWidget(self.status_label)

        right_panel.addWidget(bottom_card)

        right_panel.addStretch()
        main.addLayout(right_panel, 2)

    def apply_style(self):
        self.setStyleSheet("""
            QWidget {
                background-color: #121212;
                color: #DDDDDD;
                font-family: "Segoe UI", "Roboto", "Arial";
                font-size: 11pt;
            }
            QGroupBox#Group {
                border: 1px solid #333333;
                border-radius: 6px;
                margin-top: 10px;
            }
            QGroupBox#Group::title {
                subcontrol-origin: margin;
                left: 8px;
                padding: 0 3px 0 3px;
                color: #AAAAAA;
                font-size: 9pt;
            }
            QFrame#Card {
                border-radius: 10px;
                background-color: #1E1E1E;
            }
            QLabel#TitleLabel {
                font-size: 13pt;
                font-weight: 600;
                color: #FFFFFF;
            }
            QLabel#HeaderLabel {
                font-size: 13pt;
                font-weight: 600;
                color: #FFFFFF;
            }
            QLabel#ValueLabel {
                font-size: 11pt;
                color: #FFFFFF;
            }
            QLabel#BigValueLabel {
                font-size: 13pt;
                font-weight: 600;
                color: #7AD0FF;
            }
            QLabel#StatusLabel {
                font-size: 9pt;
                color: #BBBBBB;
            }
            QPushButton {
                background-color: #2A2A2A;
                border: 1px solid #3C3C3C;
                border-radius: 8px;
                padding: 6px 12px;
            }
            QPushButton:hover {
                background-color: #3A3A3A;
            }
            QPushButton:pressed {
                background-color: #505050;
            }
            QPushButton#QuitButton {
                background-color: #C62828;
                border: 1px solid #E53935;
                color: #FFFFFF;
                font-weight: 600;
                padding: 6px 16px;
            }
            QPushButton#QuitButton:hover {
                background-color: #E53935;
            }
            QPushButton#QuitButton:pressed {
                background-color: #B71C1C;
            }
            QSlider::groove:horizontal {
                height: 6px;
                border-radius: 3px;
                background: #333333;
            }
            QSlider::handle:horizontal {
                background: #7AD0FF;
                border-radius: 8px;
                width: 16px;
                margin: -5px 0;
            }
            QRadioButton {
                spacing: 6px;
            }
            QRadioButton::indicator {
                width: 14px;
                height: 14px;
            }
            QRadioButton::indicator::unchecked {
                border-radius: 7px;
                border: 2px solid #555555;
                background: transparent;
            }
            QRadioButton::indicator::checked {
                border-radius: 7px;
                border: 2px solid #7AD0FF;
                background: #7AD0FF;
            }
        """)

    # ------------- UI callbacks -------------
    def on_freq_change(self, value):
        self.freq_label.setText(f"{value} Hz")

    def on_amp_change(self, value):
        self.amp_label.setText(f"{value} %")

    def current_wave_str(self):
        if self.rb_sine.isChecked():
            return "SINE"
        if self.rb_tri.isChecked():
            return "TRI"
        return "SQR"

    def send_line(self, s):
        if not self.ser:
            return
        data = (s + "\r\n").encode("ascii")
        self.ser.write(data)
        self.status_label.setText(f"Status: {s}")

    def send_start(self):
        f = self.freq_slider.value()
        a = self.amp_slider.value()
        w = self.current_wave_str()
        cmd = f"FREQ:{f},AMP:{a},WAVE:{w},EN:1"
        self.send_line(cmd)

    def send_stop(self):
        self.send_line("EN:0")

    def send_meas_r(self):
        self.send_line("MEAS:R")

    def send_meas_c(self):
        self.send_line("MEAS:C")

    def quit_app(self):
        # close serial if open, then quit application
        if self.ser and self.ser.is_open:
            try:
                self.ser.close()
            except Exception:
                pass
        QtWidgets.QApplication.instance().quit()

    # ------------- Save waveform -------------
    def save_waveform(self):
        if self.last_frame is None:
            self.status_label.setText("Status: No waveform to save")
            print("No waveform to save.")
            return

        path, _ = QtWidgets.QFileDialog.getSaveFileName(
            self,
            "Save Waveform",
            "waveform.csv",
            "CSV Files (*.csv);;All Files (*)"
        )
        if not path:
            return

        frame = self.last_frame.astype(np.float32)
        t_ms = np.arange(len(frame)) / SAMPLE_RATE_HZ * 1000.0
        volts = adc_to_volts(frame)

        data = np.column_stack([t_ms, volts, frame])
        header = "t_ms,voltage_V,adc_8bit"

        try:
            np.savetxt(path, data, delimiter=",", header=header, comments="")
            self.status_label.setText("Status: Saved waveform")
            print(f"Saved waveform to {path}")
        except Exception as e:
            self.status_label.setText(f"Status: Save failed: {e}")
            print("Error saving waveform:", e)

    # ------------- serial polling -------------
    def poll_serial(self):
        if not self.ser:
            return

        while True:
            n = self.ser.in_waiting
            if n <= 0:
                break
            b = self.ser.read(1)
            if not b:
                break
            self.handle_byte(b[0])

    def handle_byte(self, b):
        # frame state machine
        if self.frame_state == "idle":
            if b == 0xAA:
                self.frame_state = "len"
            else:
                ch = chr(b)
                if ch == '\r' or ch == '\n':
                    if self.line_buf:
                        self.handle_line(self.line_buf)
                        self.line_buf = ""
                elif 32 <= b <= 126:
                    self.line_buf += ch
        elif self.frame_state == "len":
            self.pending_len = b
            if self.pending_len == FRAME_SAMPLES:
                self.pending_data = bytearray()
                self.frame_state = "data"
            else:
                self.frame_state = "idle"
        elif self.frame_state == "data":
            self.pending_data.append(b)
            if len(self.pending_data) >= self.pending_len:
                self.handle_frame(bytes(self.pending_data))
                self.frame_state = "idle"

    def handle_line(self, line):
        if line.startswith("R_GND:"):
            try:
                r = int(line.split(":", 1)[1])
                self.label_R.setText(f"R (Ω): {r}")
            except ValueError:
                pass
        elif line.startswith("C_uF:"):
            try:
                c = float(line.split(":", 1)[1])
                self.label_C.setText(f"C (µF): {c:.3f}")
            except ValueError:
                pass
        elif line.startswith("READY"):
            self.status_label.setText("Status: READY")
        elif line.startswith("DBG_"):
            print(line)
        else:
            self.status_label.setText(f"Status: {line}")

    def handle_frame(self, data):
        frame = np.frombuffer(data, dtype=np.uint8)
        self.last_frame = frame

        aligned_v = trigger_align(frame, N_PLOT)
        self.curve.setData(self.t, aligned_v)

        freq, amp = estimate_freq_amp(frame)
        self.plot.setTitle(f"Freq: {freq:7.1f} Hz    Amp: {amp:5.3f} Vpp")


# ---------- signal processing helpers ----------

def adc_to_volts(arr):
    return (arr.astype(np.float32) / 255.0) * FULL_SCALE_V * CAL_GAIN

def trigger_align(frame, n_out):
    vals_adc = frame.astype(np.float32)
    vals_v = adc_to_volts(frame)

    vmin_adc, vmax_adc = vals_adc.min(), vals_adc.max()
    p2p_adc = vmax_adc - vmin_adc

    if p2p_adc < 3:
        return vals_v[:n_out]

    thr = vmin_adc + p2p_adc / 2.0
    above = vals_adc > thr
    crossings = np.where((~above[:-1]) & (above[1:]))[0]

    if len(crossings) == 0:
        return vals_v[:n_out]

    start = crossings[0]
    end = start + n_out

    if end <= len(vals_v):
        return vals_v[start:end]
    else:
        out = np.zeros(n_out, dtype=np.float32)
        sl = len(vals_v) - start
        out[:sl] = vals_v[start:]
        out[sl:] = vals_v[-1]
        return out

def estimate_freq_amp(frame):
    vals_adc = frame.astype(np.float32)
    vmin_adc, vmax_adc = vals_adc.min(), vals_adc.max()
    p2p_adc = vmax_adc - vmin_adc
    amp_vpp = (p2p_adc / 255.0) * FULL_SCALE_V * CAL_GAIN

    if p2p_adc < 3:
        return 0.0, amp_vpp

    thr = vmin_adc + p2p_adc / 2.0
    above = vals_adc > thr
    crossings = np.where((~above[:-1]) & (above[1:]))[0]

    if len(crossings) < 2:
        return 0.0, amp_vpp

    periods = np.diff(crossings).astype(np.float32)
    mean_period = periods.mean()
    freq = SAMPLE_RATE_HZ / mean_period
    return freq, amp_vpp


# ---------- main ----------
if __name__ == "__main__":
    app = QtWidgets.QApplication([])
    win = ScopeFuncGenRC()
    win.show()
    app.exec_()
