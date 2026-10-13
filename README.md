# Intelligent Robotic Arm for Color-Based Object Sorting

An academic project developed at the **Holon Institute of Technology (HIT)** as part of the **Robotics & Software Engineering** course.  
The project demonstrates how **low-cost hardware** and **AI-based vision** can be integrated to create an **autonomous robotic arm** capable of sorting objects by color.  

The system combines **Arduino**, **ESP32**, and the **Seeed Grove Vision AI** module, with a **React web interface** for manual control and calibration.

## Installation

### Hardware Requirements
- Arduino Uno WiFi Rev2  
- ESP32-C3 (for communication and AI data processing)  
- Seeed Grove Vision AI Module  
- Six servo motors (waist, shoulder, elbow, wrist roll, wrist pitch, gripper)  
- 3D-printed robotic arm structure  
- Power supply and connecting wires  

### Software Requirements
- **Arduino IDE** (for uploading firmware to Arduino and ESP32)  
- **Node.js and npm** (for running the React web interface)  
- **Vite** (for local server development)

- ## Usage

### Overview

The system supports two main modes of operation:

#### 1. Automatic Mode
- The **Grove Vision AI camera** detects the color of an object placed in front of it.  
- The **ESP32** processes this data and transmits it to the **Arduino**, which executes a sequence of servo movements.  
- The robotic arm performs the following actions:
  1. Picks up the object.  
  2. Places it in the corresponding color bin.  
  3. Returns to the base position.  

#### 2. Manual Mode
Through the **React web interface**, the user can:
- Control each servo using sliders.  
- Save, view, and delete **custom servo positions**.  
- Execute **preset actions** such as *Go to Base*, *Catch Cube*, and *Drop into Bin*.  
- Monitor **real-time logs**, color detection data, and communication history.

### Demonstration
- **Full Project Video:** [YouTube – Intelligent Robotic Arm](https://youtu.be/WaWYLHGAR-I)
  
---

## Contributing

Contributions and improvements are welcome.  
Please follow these guidelines:

1. Fork the repository.  
2. Create a new branch for your feature or bug fix.  
3. Commit changes with clear messages.  
4. Submit a pull request describing your contribution.  

Ensure that any proposed changes maintain the modular and educational nature of the project.

---

## Acknowledgments

**Supervisor:** Dvir Chakim  
**Authors:** Oshri Halevi, Adi Haim, Afik Aharon. 
**Institution:** Holon Institute of Technology (HIT)

### Inspiration
- Real-world applications in industrial sorting and automation.  
- The motivation to demonstrate how theoretical computer science concepts can be applied to practical robotics.

### Libraries and Frameworks
- Arduino C++ Libraries (Servo, WiFiNINA)  
- ESP32 SDK (WiFi, HTTP Client)  
- Seeed Grove Vision AI SDK  
- React, Node.js, Vite  

### References
- Sommerville, I. (2016). *Software Engineering* (10th ed.). Pearson.  
- Gonzalez, R. C., & Woods, R. E. (2018). *Digital Image Processing*. Pearson.  
- [HowToMechatronics – DIY Arduino Robot Arm](https://howtomechatronics.com/tutorials/arduino/diy-arduino-robot-arm-with-smartphone-control/)  
- [Printables – 3D Printed Holder for Grove Vision AI](https://www.printables.com/model/1250656-3d-printed-foldable-holder-for-grove-vision-ai-mod)  
- [SenseCraft AI Platform](https://sensecraft.seeed.cc/ai/home)  
- ChatGPT (OpenAI Assistance)


