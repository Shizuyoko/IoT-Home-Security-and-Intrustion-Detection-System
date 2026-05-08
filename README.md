## IoT Home Security & Intrusion Detection System

A smart, low‑power home security system built using the ESP32 platform, featuring motion detection, door detection, motion triggered snapshots, LED indicators, LCD alarm-state messages and an audible buzzer alarm. The system is designed as a complete end‑to‑end IoT project, containing ESP32 firmware, backend alert‑handling services, and a web‑based dashboard for monitoring events.



 ### <img src="https://cdn.simpleicons.org/espressif/red" width="40" /> Espressif ESP32 DevKit

The ESP32 serves as the central microcontroller for the system, coordinating all sensor inputs, device outputs, and communication flows. It continuously monitors the PIR motion sensor, door switch, and keypad, processes events locally, and triggers visual or audible alerts when required. Its dual‑core architecture allows the device to handle real‑time tasks—such as reading sensors and updating alarm states—while simultaneously managing Wi‑Fi communication with the backend. The ESP32 sends structured JSON payloads to the server whenever an event occurs, enabling reliable, low‑latency integration with the cloud dashboard. With built‑in Wi‑Fi, low power consumption, and a rich GPIO feature set, the ESP32 is ideal for responsive, always‑connected IoT security applications.

## <img src="https://cdn.jsdelivr.net/npm/heroicons@2.1.1/24/outline/video-camera.svg" width="32" /> ESP32-CAM
The ESP32‑CAM provides the visual monitoring capability of the system, delivering both a live MJPEG video stream and on‑demand snapshot capture. It connects to the same Wi‑Fi network as the main ESP32 controller and exposes lightweight HTTP endpoints that allow the backend to request images whenever motion or door events occur. This module operates independently from the alarm controller, ensuring reliable image capture without interrupting sensor processing. Its compact form factor and built‑in OV2640 camera make it ideal for low‑cost, real‑time intrusion detection in IoT environments.

### <img src="https://cdn.jsdelivr.net/npm/heroicons@2.1.1/24/outline/wrench-screwdriver.svg" width="28" /> Hardware Components 



▶️ **Breadboard :** 

Serves as the foundation for the initial hardware prototype, allowing the circuit to be assembled, tested, and refined before moving to a permanent layout. It provides a flexible environment for connecting the ESP32, sensors, and output components while validating the system’s behaviour during early development.


▶️ **Motion Sensor (PIR):**

The system incorporates a dedicated motion‑detection sensor to monitor activity within the environment, allowing the ESP32 to respond instantly whenever movement is detected. This component forms the core of the security logic, enabling real‑time alerts and automated system behaviour.


▶️ **Alarm (Piezo) :**

The piezo alarm provides the system’s audible alert, activating whenever motion is detected or when the device enters an alarm state. Its sharp, high‑frequency tone ensures that any triggered event is immediately noticeable during both testing and real‑world operation. (Alarm is triggered every 1,000 milliseconds during testing)..


▶️ **RGB LED :**

The RGB LED acts as the system’s primary alarm status and alert indicator, using colour to communicate the current operating mode.

- Green → System armed and monitoring

- Red → Alarm active

- Yellow → System disarmed

This provides an immediate visual reference for the user, even at a distance.

▶️ **LCD (Liquid Crystal Display) :**

The 16×2 LCD module displays real‑time system information such as arming state, motion alerts, door status, and keypad input. It allows the user to interact with the system without needing a phone or computer, making the device fully standalone and easy to operate.

▶️ **KEYPAD :**

The 4×4 keypad enables secure user input for arming and disarming the system. It supports PIN‑based authentication, ensuring that only authorised users can change the system state. The LCD is also fitted with a CLEAR INPUT feature, if the initial input had been entered incorrectly, by the current user. 
