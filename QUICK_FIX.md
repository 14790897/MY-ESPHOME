# 🚨 ESPHome 快速故障排除

## 🔧 常见问题快速修复

### 1. PlatformIO 路径错误
**错误**: `FileNotFoundError: 'D:\\'`

**快速修复**:
```powershell
$env:PLATFORMIO_CORE_DIR = "C:\Users\$env:USERNAME\.platformio"
```

### 2. 文件包含路径错误  
**错误**: `No such file or directory: common.yaml`

**修复方法**:
```yaml
packages:
  base: !include ../../common.yaml  # 检查相对路径层级
```

### 3. 编译框架错误
**问题**: esp-idf 框架兼容性问题

**推荐配置**:
```yaml
esp32:
  framework:
    type: arduino  # 更稳定
```

### 4. 舵机频率错误
**问题**: 舵机不正常工作

**正确配置**:
```yaml
output:
  - platform: ledc
    frequency: 50Hz  # 标准舵机频率
```

### 5. GPIO 引脚警告
**警告**: GPIO2 is a strapping PIN

**推荐引脚 (ESP32-C3)**:
- ✅ 使用: GPIO4, GPIO5, GPIO6, GPIO7  
- ❌ 避免: GPIO0, GPIO1, GPIO2

---

## 🆘 紧急重置
```powershell
# 完全重置 PlatformIO
Remove-Item -Recurse -Force "$env:USERPROFILE\.platformio"
pip install -U platformio
```

## 📖 详细指南
查看 `docs/TROUBLESHOOTING.md` 获取完整解决方案。