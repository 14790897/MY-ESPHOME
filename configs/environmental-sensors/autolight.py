import requests
import subprocess
import time
import sys

# ================= 配置区域 =================

# 传感器地址
SENSOR_URL = "http://temt6000-sensor.local/sensor/temt6000_percentage"

# Twinkle Tray 的安装路径 (请根据实际情况修改)
# 如果是默认安装，通常是这个:
TT_PATH = r"C:\Users\13963\AppData\Local\Programs\twinkle-tray\Twinkle Tray.exe"

# 刷新间隔 (秒)
INTERVAL = 5

# 屏幕亮度限制
# MIN_BRIGHTNESS: 哪怕传感器是 0%，屏幕最低也保持在这个亮度，防止黑屏
MIN_BRIGHTNESS = 10 
MAX_BRIGHTNESS = 100

# 灵敏度阈值：只有变化超过这个值才调节，避免屏幕忽明忽暗
THRESHOLD = 3 

# ===========================================

def get_sensor_value():
    """解析 ESPHome JSON 数据"""
    try:
        response = requests.get(SENSOR_URL, timeout=3)
        response.raise_for_status()
        
        data = response.json()
        
        # 针对你的数据格式 {"id":..., "value":0.836364, ...}
        if 'value' in data:
            val = float(data['value'])
            print(f"当前环境亮度: {val:.2f}%") # 打印出来方便调试
            return val
            
    except Exception as e:
        print(f"获取数据失败: {e}")
        return None
    
    return None

def set_screen_brightness(level):
    """调用 Twinkle Tray"""
    # 限制范围：确保数值在 10 - 100 之间
    safe_level = max(MIN_BRIGHTNESS, min(MAX_BRIGHTNESS, level))
    safe_level = int(safe_level)

    # 这里的 --Set=XX 会同步调节所有连接的显示器
    cmd = [TT_PATH,"--All", f"--Set={safe_level}"]
    
    try:
        # 隐藏 CMD 黑框
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        
        subprocess.run(cmd, startupinfo=startupinfo)
        print(f"-> 执行调节: 屏幕已设为 {safe_level}%")
    except FileNotFoundError:
        print(f"错误: 找不到文件 {TT_PATH}")

def main():
    print("--- 自动亮度脚本运行中 (按 Ctrl+C 停止) ---")
    last_brightness = -1

    while True:
        sensor_val = get_sensor_value()

        if sensor_val is not None:
            # 逻辑：直接把环境亮度的百分比给屏幕
            # 例如环境 0.8%，屏幕会触发 MIN_BRIGHTNESS (10%)
            # 例如环境 50%，屏幕设为 50%
            target_brightness = sensor_val

            # 防抖动判断
            if abs(target_brightness - last_brightness) > THRESHOLD:
                set_screen_brightness(target_brightness)
                last_brightness = target_brightness
            else:
                # 变化太小，不调节
                pass
        
        time.sleep(INTERVAL)

if __name__ == "__main__":
    main()