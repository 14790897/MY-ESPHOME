# MQTT TLS Connection Fails on ESP32-C3 but Works on Linux

## Environment
- **ESPHome Version**: Latest (ESP-IDF framework)
- **Device**: ESP32-C3 (airm2m_core_esp32c3)
- **Framework**: ESP-IDF
- **MQTT Broker**: OneNET (China Mobile IoT Platform)
  - Host: `nkKRY36VE6.mqttstls.acc.cmcconenet.cn`
  - Port: `8883` (TLS)

## Problem Description
MQTT over TLS connection fails on ESP32-C3 with ESPHome, but the **exact same credentials work successfully on Linux** using `mosquitto_sub`. The connection briefly succeeds (CONNACK received) but immediately disconnects with a TLS error.

## Error Log (ESP32-C3)
```
[20:04:29.003][I][mqtt:268]: Connecting
[20:04:30.119][D][main:323]: ✓✓✓ MQTT连接成功！✓✓✓
[20:04:30.119][I][mqtt:094]: MQTT已连接到OneNET
[20:04:30.119][W][component:326]: mqtt cleared Warning flag
[20:04:30.119][I][mqtt:309]: Connected
[20:04:30.244][W][component:443]: mqtt took a long time for an operation (115 ms)
[20:04:30.244][W][component:446]: Components should block for at most 30 ms
[20:04:30.324][E][mqtt.idf:167]: MQTT_EVENT_ERROR
[20:04:30.324][E][mqtt.idf:169]: Last error code reported from esp-tls: 0x8008
[20:04:30.324][E][mqtt.idf:170]: Last tls stack error number: 0x0
[20:04:30.324][E][mqtt.idf:171]: Last captured errno : 0 (Success)
[20:04:30.324][D][esp-idf:000][mqtt_task]: E (15263) mqtt_client: esp_mqtt_handle_transport_read_error: transport_read(): EOF
[20:04:30.324][D][esp-idf:000][mqtt_task]: E (15264) mqtt_client: esp_mqtt_handle_transport_read_error: transport_read() error: errno=119
[20:04:30.324][D][esp-idf:000][mqtt_task]: E (15264) mqtt_client: mqtt_process_receive: mqtt_message_receive() returned -2
[20:04:30.324][D][main:101]: ✗ MQTT断开连接
[20:04:30.324][W][mqtt:358]: Disconnected: TCP disconnected
```

**Key Error**: `esp-tls: 0x8008` - This indicates a TLS read error, followed by EOF and TCP disconnection.

## Successful Connection on Linux
The same credentials work perfectly on Linux using `mosquitto_sub`:

```bash
mosquitto_sub -h nkKRY36VE6.mqttstls.acc.cmcconenet.cn -p 8883 \
  --cafile onenet.pem --insecure \
  -i mypanel -u nkKRY36VE6 \
  -P 'version=2018-10-31&res=products%2FnkKRY36VE6%2Fdevices%2Fmypanel&et=1791185468&method=md5&sign=v%2BxH6eBLz%2BI1NXNgzieDcA%3D%3D' \
  -t '$sys/nkKRY36VE6/mypanel/#' \
  -k 60 -d
```

**Output**:
```
Client mypanel sending CONNECT
Client mypanel received CONNACK (0)
Client mypanel sending SUBSCRIBE (Mid: 1, Topic: $sys/nkKRY36VE6/mypanel/#, QoS: 0, Options: 0x00)
Client mypanel received SUBACK
Subscribed (mid: 1): 0
[Connection remains stable and receives messages]
```

## ESPHome Configuration

```yaml
mqtt:
  broker: "nkKRY36VE6.mqttstls.acc.cmcconenet.cn"
  port: 8883
  username: "nkKRY36VE6"
  password: "version=2018-10-31&res=products%2FnkKRY36VE6%2Fdevices%2Fmypanel&et=1791185468&method=md5&sign=v%2BxH6eBLz%2BI1NXNgzieDcA%3D%3D"
  client_id: "mypanel"
  clean_session: true
  topic_prefix: null
  enable_on_boot: false
  discovery: false
  discovery_retain: false
  skip_cert_cn_check: true
  idf_send_async: false
  keepalive: 60s
  log_topic: null
  birth_message:
  will_message:
  
  certificate_authority: |-
    -----BEGIN CERTIFICATE-----
    MIIDOzCCAiOgAwIBAgIJAPCCNfxANtVEMA0GCSqGSIb3DQEBCwUAMDQxCzAJBgNV
    BAYTAkNOMQ4wDAYDVQQKDAVDTUlPVDEVMBMGA1UEAwwMT25lTkVUIE1RVFRTMB4X
    DTE5MDUyOTAxMDkyOFoXDTQ5MDUyMTAxMDkyOFowNDELMAkGA1UEBhMCQ04xDjAM
    BgNVBAoMBUNNSU9UMRUwEwYDVQQDDAxPbmVORVQgTVFUVFMwggEiMA0GCSqGSIb3
    DQEBAQUAA4IBDwAwggEKAoIBAQC/VvJ6lGWfy9PKdXKBdzY83OERB35AJhu+9jkx
    5d4SOtZScTe93Xw9TSVRKrFwu5muGgPusyAlbQnFlZoTJBZY/745MG6aeli6plpR
    r93G6qVN5VLoXAkvqKslLZlj6wXy70/e0GC0oMFzqSP0AY74icANk8dUFB2Q8usS
    UseRafNBcYfqACzF/Wa+Fu/upBGwtl7wDLYZdCm3KNjZZZstvVB5DWGnqNX9HkTl
    U9NBMS/7yph3XYU3mJqUZxryb8pHLVHazarNRppx1aoNroi+5/t3Fx/gEa6a5PoP
    ouH35DbykmzvVE67GUGpAfZZtEFE1e0E/6IB84PE00llvy3pAgMBAAGjUDBOMB0G
    A1UdDgQWBBTTi/q1F2iabqlS7yEoX1rbOsz5GDAfBgNVHSMEGDAWgBTTi/q1F2ia
    bqlS7yEoX1rbOsz5GDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAL
    aqJ2FgcKLBBHJ8VeNSuGV2cxVYH1JIaHnzL6SlE5q7MYVg+Ofbs2PRlTiWGMazC7
    q5RKVj9zj0z/8i3ScWrWXFmyp85ZHfuo/DeK6HcbEXJEOfPDvyMPuhVBTzuBIRJb
    41M27NdIVCdxP6562n6Vp0gbE8kN10q+ksw8YBoLFP0D1da7D5WnSV+nwEIP+F4a
    3ZX80bNt6tRj9XY0gM68mI60WXrF/qYL+NUz+D3Lw9bgDSXxpSN8JGYBR85BxBvR
    NNAhsJJ3yoAvbPUQ4m8J/CoVKKgcWymS1pvEHmF47pgzbbjm5bdthlIx+swdiGFa
    WzdhzTYwVkxBaU+xf/2w
    -----END CERTIFICATE-----
```

## Credentials (Test Account)
- **Broker**: nkKRY36VE6.mqttstls.acc.cmcconenet.cn:8883
- **Product ID**: nkKRY36VE6
- **Device ID**: mypanel
- **Username**: nkKRY36VE6
- **Password**: `version=2018-10-31&res=products%2FnkKRY36VE6%2Fdevices%2Fmypanel&et=1791185468&method=md5&sign=v%2BxH6eBLz%2BI1NXNgzieDcA%3D%3D`
- **Client ID**: mypanel

## Analysis

1. **Connection is established**: The ESP32 successfully completes the TLS handshake and receives CONNACK
2. **Immediate disconnection**: Right after connection, a TLS read error occurs (`0x8008`)
3. **EOF received**: The broker appears to close the connection immediately
4. **Platform difference**: Identical credentials work on Linux but fail on ESP-IDF

## Things Tried
- ✅ `skip_cert_cn_check: true` - No effect
- ✅ `idf_send_async: false` - No effect
- ✅ Verified certificate matches OneNET's official certificate
- ✅ Confirmed credentials are correct (work on Linux)
- ✅ Set `clean_session: true`
- ✅ Removed birth/will messages

## Questions

1. **Is this an ESP-IDF MQTT client issue?** Why does the connection close immediately after CONNACK only on ESP32?
2. **Could this be related to TLS version negotiation?** Does ESP-IDF use a different TLS version than desktop Linux?
3. **Is there a timeout or buffer size issue?** The warning about "took a long time for an operation (115 ms)" appears just before the error.
4. **Are there additional MQTT/TLS settings needed for ESP-IDF** that aren't required on desktop platforms?

## Expected Behavior
The MQTT connection should remain stable after CONNACK, just like it does with `mosquitto_sub` on Linux.

## Actual Behavior
Connection is established but immediately drops with TLS error `0x8008` (EOF).

## Additional Context
- WiFi connection is stable
- Time synchronization is successful
- API service works correctly
- The same configuration pattern works on other MQTT brokers (tested locally)

---

**Is this a known compatibility issue between ESP-IDF's MQTT client and certain MQTT brokers?** Any guidance would be greatly appreciated.
