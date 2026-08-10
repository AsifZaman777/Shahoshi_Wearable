import os
import sys
import time
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[LOG SERVICE] Installing required 'pyserial' package...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyserial"])
    import serial
    import serial.tools.list_ports

def get_available_ports():
    return list(serial.tools.list_ports.comports())

def auto_detect_port():
    ports = get_available_ports()
    for p in ports:
        desc = p.description.upper()
        if any(keyword in desc for keyword in ["CP210", "CH340", "USB", "UART", "ESP"]):
            return p.device
    if ports:
        non_com1 = [p.device for p in ports if p.device != "COM1"]
        if non_com1:
            return non_com1[0]
        return ports[0].device
    return None

class DayWiseLoggerService:
    def __init__(self, port, baud=115200, logs_dir="logs"):
        self.port = port
        self.baud = baud
        self.logs_dir = os.path.abspath(logs_dir)
        os.makedirs(self.logs_dir, exist_ok=True)
        self.current_date_str = ""
        self.current_file = None

    def get_log_file_path(self, date_str):
        return os.path.join(self.logs_dir, f"log_{date_str}.log")

    def write_log(self, text_line):
        today_str = datetime.now().strftime("%Y-%m-%d")
        time_str = datetime.now().strftime("%H:%M:%S")

        if today_str != self.current_date_str:
            if self.current_file:
                self.current_file.write(f"[{time_str}] === Log Rollover to {today_str} ===\n")
                self.current_file.close()
            
            self.current_date_str = today_str
            log_path = self.get_log_file_path(today_str)
            file_exists = os.path.exists(log_path)
            self.current_file = open(log_path, "a", encoding="utf-8")
            
            if not file_exists:
                self.current_file.write(f"[{time_str}] === Day-Wise Log Service Started for {today_str} ===\n")

        formatted_entry = f"[{time_str}] {text_line}\n"
        self.current_file.write(formatted_entry)
        self.current_file.flush()

    def run(self):
        print(f"\n==================================================")
        print(f"  SHAHOSHI WEARABLE - DAY-WISE LOG SERVICE")
        print(f"==================================================")
        print(f"Target Port : {self.port} @ {self.baud} baud")
        print(f"Log Directory: {self.logs_dir}")

        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
            print(f"[SERVICE RUNNING] Connected to {self.port}. Monitoring live serial output...\n")
        except Exception as e:
            print(f"[ERROR] Could not open serial port {self.port}: {e}")
            print("\nAvailable COM Ports:")
            for p in get_available_ports():
                print(f" - {p.device}: {p.description}")
            sys.exit(1)

        try:
            while True:
                line = ser.readline()
                if line:
                    decoded = line.decode('utf-8', errors='replace').rstrip()
                    if decoded:
                        print(decoded)
                        self.write_log(decoded)
        except KeyboardInterrupt:
            print("\n[SERVICE] Log service stopped by user.")
        finally:
            if self.current_file:
                self.current_file.close()
            ser.close()

def main():
    if len(sys.argv) > 1 and sys.argv[1].strip():
        port = sys.argv[1].strip()
    else:
        port = auto_detect_port()

    if not port:
        print("[ERROR] No active COM port detected. Please specify COM port:")
        print("Usage: python log_service.py COM3")
        sys.exit(1)

    service = DayWiseLoggerService(port=port)
    service.run()

if __name__ == "__main__":
    main()
