Xbox Controller-Based Wi-Fi Car Control System
This project enables real-time wireless control of a robotic car using an Xbox controller over a TCP/IP connection. It consists of two main components:

Python Host App: A Python script using pygame captures joystick inputs, processes them into motor control commands (speed and direction), and sends them to the ESP32-based car over a TCP connection.

ESP32 Firmware: Embedded C code running on an ESP32 receives the control data, interprets motor commands, and controls the car's motors via GPIO and PWM using FreeRTOS tasks.

Features:
Xbox controller support via pygame

Multithreaded TCP server for real-time communication

Adjustable motor speed control

ESP32-side motor logic and PWM control

Modular, responsive, and low-latency system

This project is ideal for robotics, embedded systems learning, or remote vehicle control over Wi-Fi
