## <img src="https://cdn.jsdelivr.net/npm/heroicons@2.1.1/24/outline/video-camera.svg" width="32" /> IoT Home Security & Intrusion Detection System

A smart, low‑power home security system built using the ESP32 platform, featuring motion detection, LED indicators, and an audible alarm. The system is designed as a full‑stack IoT solution, with firmware running on the ESP32, backend services for alert handling, and a user‑facing dashboard for monitoring events.



 ### <img src="https://cdn.simpleicons.org/espressif/red" width="40" /> Espressif ESP32

The ESP32 serves as the core microcontroller for the system, handling sensor input, device control, and communication with backend services. Its built‑in Wi‑Fi capabilities make it ideal for IoT applications requiring real‑time monitoring and remote interaction.

### <img src="https://cdn.jsdelivr.net/npm/heroicons@2.1.1/24/outline/wrench-screwdriver.svg" width="28" /> Hardware Components 



▶️ **Breadboard :** 

Serves as the foundation for the initial hardware prototype, allowing the circuit to be assembled, tested, and refined before moving to a permanent layout. It provides a flexible environment for connecting the ESP32, sensors, and output components while validating the system’s behaviour during early development.


▶️ **Sensors (PIR):**

The system incorporates a dedicated motion‑detection sensor to monitor activity within the environment, allowing the ESP32 to respond instantly whenever movement is detected. This component forms the core of the security logic, enabling real‑time alerts and automated system behaviour.


▶️ **LEDs :**

The LEDs provide clear visual feedback during system operation, indicating states such as power/alarm state, motion detection, and alert activation. They play a key role in making the system’s behaviour easy to understand at a glance during both testing and real‑world use.


▶️ **Alarm (Piezo) :**

The piezo alarm provides the system’s audible alert, activating whenever motion is detected or when the device enters an alarm state. Its sharp, high‑frequency tone ensures that any triggered event is immediately noticeable during both testing and real‑world operation. (Alarm is triggered every 1,000 milliseconds during testing)..


