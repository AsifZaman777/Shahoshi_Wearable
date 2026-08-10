import os

try:
    Import("env")
    project_dir = env.get("PROJECT_DIR")
except NameError:
    # Standalone execution
    project_dir = os.path.dirname(os.path.abspath(__file__))

env_path = os.path.join(project_dir, ".env")
config_header_path = os.path.join(project_dir, "include", "config.h")

env_vars = {}
if os.path.exists(env_path):
    with open(env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                key, val = line.split("=", 1)
                env_vars[key.strip()] = val.strip().strip('"').strip("'")

header_content = f"""#ifndef CONFIG_H
#define CONFIG_H

// Auto-generated from .env by load_env.py - DO NOT COMMIT TO GITHUB
#define BREVO_API_KEY_VAL "{env_vars.get('BREVO_API_KEY', 'YOUR_BREVO_API_KEY')}"
#define ALERT_SENDER_NAME_VAL "{env_vars.get('ALERT_SENDER_NAME', 'Shahoshi Wearable System')}"
#define ALERT_SENDER_EMAIL_VAL "{env_vars.get('ALERT_SENDER_EMAIL', 'alert@shahoshi.local')}"
#define ALERT_RECIPIENT_NAME_VAL "{env_vars.get('ALERT_RECIPIENT_NAME', 'Emergency Contact')}"
#define ALERT_RECIPIENT_EMAIL_VAL "{env_vars.get('ALERT_RECIPIENT_EMAIL', 'emergency@shahoshi.local')}"
#define WIFI_SSID_VAL "{env_vars.get('WIFI_SSID', 'Wokwi-GUEST')}"
#define WIFI_PASS_VAL "{env_vars.get('WIFI_PASS', '')}"

#endif
"""

os.makedirs(os.path.dirname(config_header_path), exist_ok=True)
with open(config_header_path, "w", encoding="utf-8") as f:
    f.write(header_content)

print(f"[load_env.py] Successfully loaded environment from .env into {config_header_path}")
