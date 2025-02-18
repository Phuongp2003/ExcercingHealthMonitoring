#unuse
import numpy as np
from scipy.signal import find_peaks

class SignalProcessor:
    def __init__(self):
        self.signal_data = []

    def process_signal(self, ir_signal, red_signal, green_signal):
        ir_signal = np.array(ir_signal)
        red_signal = np.array(red_signal)
        green_signal = np.array(green_signal)
        
        try:
            # Tính BPM (Heart Rate)
            peaks, _ = find_peaks(ir_signal, distance=50)
            
            if len(peaks) < 2:
                return None, None
            
            peak_times = peaks / 100  # Tính thời gian giữa các đỉnh (giả sử lấy mẫu 100 Hz)
            avg_period = np.mean(np.diff(peak_times))
            bpm = 60 / avg_period
            
            # Tính SpO2 (Oxygen Saturation)
            ir_avg = np.mean(ir_signal)
            red_avg = np.mean(red_signal)
            
            R = red_avg / ir_avg
            spo2 = 110 - 25 * R
            
            return bpm, spo2
        except Exception as e:
            print(f"Error processing signal: {e}")
            return None, None
