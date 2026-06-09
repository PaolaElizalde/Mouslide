# Mouslide
This is a mouse designed for people with an upper-body disability for the InnovaTec EXPO 2026. It consists of a large 3D-printed case with 6 large buttons, each with a different function, wired up with an ESP32 with 32 pins. 
# About the Project
This project is an innovative solution developed for the Instituto Tecnológico de Pachuca (TecNM). This project is the brainchild of a brilliant team of four creators from the Instituto Tecnológico de Pachuca. My role in this project is as an IT Student, assisting the team with technical implementation, wiring the hardware/software components, and coding the functional prototype to bring their vision to life. All conceptual credit belongs entirely to them.
Meet the Team & Learn More. To see the official project stand, learn more about the concept, and meet the incredibly talented creators behind this idea, please visit the official TecNM Pachuca social media coverage: Instituto Tecnológico de Pachuca on Facebook (https://www.facebook.com/p/TecNM-Pachuca-61557528505129/?locale=es_LA) 
# Implementation
 I utilised the following technologies to build the system (Feel free to modify this section with the actual tech you are using!):
 - ArduinoIDE with C++ (libraries available in the repository)

LIBRARIES:
Core ESP32 Go to Preferences → URL Install  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
BleMouse — download the ZIP from github.com/T-vK/ESP32-BLE-Mouse and add it with  Sketch → Include Library → Add .ZIP
Adafruit MPU6050 & Adafruit Unified Sensor — download directly from the library manager in Arduino IDE.

DRIVER:
To know which one to use, look at the small chip next to the USB port of your ESP-32 
According to your plaque, use either: 
CP2102 (common): https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
CH340: search "CH340 driver Windows" on Google → sitio wch.cn

MATERIALS:
1. ESP32 DevKit V1 principal
WiFi & Bluetooth

2. HW-357 Module 
It charges the battery and gives 5V to the ESP
OR USE 
TP4056 + Boost converter MT3608

3. LiPo 3.7V 1000mAh Battery 
with JST-PH connector of 2 pins (red=positive, black=negative).

4. MPU-6050 sensor OPTIONAL
If you want to move the mouse, the cursor will move using this one.

5. 6×TTP223B buttons
Each for each button.

CONNECTIONS: 
On the code and on the diagram Diagrama de Conexiones.jpg 
