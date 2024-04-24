# ESP32-BLE-Recon
Simple ESP32 BLE Recon over Serial.  Provides a parseable JSON stream.

## Contents
`config.yaml`

## Description
**Baud**: `115200`  
**JSON Fields**:
- `ID`: ESP32's MAC Address
- `FW`: Firmware version from config.yaml
- `TYPE`: BT, BLE, iBeacon, Eddystone
- `UUID`: Unique BT Address, if applicable
- `MFR`:  Manufacturer, if found
- `NAME`: Bluetooth Device Name
- `MAC`: MAC Address
- `RSSI`:  Signal Strength
- `TX`: Reported TX power at 1 Meter, if broadcasted.

## Flashing
#### Flashing via Web Interface
Check out [update.devkitty.io](https://update.devkitty.io) for instructions.
#### Flashing via Command Line
**Install Dependencies**
```
sudo apt install python3 python3-pip
pip3 install esptool
```
**Download the latest binary & flash**
```
wget <>
python3 -m esptool write_flash 0  *ESP32-BLE-Recon*.bin
```

