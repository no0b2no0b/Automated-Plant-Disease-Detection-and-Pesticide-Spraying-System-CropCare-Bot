🌱 CropCare Bot - Autonomous Plant Disease Detection and Smart Spraying System
📌 Overview

CropCare Bot is an intelligent agricultural robot designed to detect plant diseases in real time using Computer Vision and Deep Learning. The system automatically classifies plant diseases from leaf images, recommends suitable treatment, and activates the appropriate pesticide spraying mechanism. The robot can be remotely controlled through an ESP32-based Wi-Fi interface, making it suitable for smart agriculture applications.

🎯 Objectives
Early detection of plant diseases.
Reduce crop loss caused by delayed diagnosis.
Automate pesticide recommendation and spraying.
Minimize human exposure to harmful chemicals.
Promote precision agriculture through targeted treatment.

🚀 Features

Computer Vision Based Disease Detection
- Real-time image acquisition using camera.
- CNN-based disease classification.
- Disease prediction with confidence score.
- Support for Tomato and Potato plant diseases.
  
Smart Spraying Mechanism
- Automatic pesticide recommendation.
- Multiple pesticide tanks and pumps.
- Targeted spraying based on detected disease.
- Reduced pesticide wastage.
  
Mobile Robot Platform
- Differential drive mobile robot.
- ESP32-based wireless control.
- Smartphone web interface control.
- Suitable for agricultural field navigation.

  WebPage :
  <img width="755" height="1600" alt="1" src="https://github.com/user-attachments/assets/4863e12d-05d7-47a3-88be-ed0e1ba79119" />
<img width="763" height="1600" alt="2" src="https://github.com/user-attachments/assets/a040073b-fd8d-4d63-94a3-ddac3ec2a416" />



🧠 Software Architecture

Image Acquisition
        ↓
Image Preprocessing
        ↓
CNN Model
(Feature Extraction + Classification)
        ↓
Disease Prediction
        ↓
Decision Logic
        ↓
ESP32 Communication
        ↓
Pump Activation & Spraying

🔬 Computer Vision Pipeline

Live Image Capture
        ↓
Preprocessing
(Resize, Normalize)
        ↓
Feature Extraction
(CNN Convolution Layers)
        ↓
Disease Classification
        ↓
Confidence Score Generation
        ↓
Pesticide Recommendation

Electronic Diagram

<img width="671" height="496" alt="Fig3 4" src="https://github.com/user-attachments/assets/434a06d5-ae73-4b01-a265-819b57a994f2" />


🎮 Robot Control

The robot is controlled wirelessly through a smartphone using the ESP32 Wi-Fi module.

Working
ESP32 connects to Wi-Fi.
ESP32 generates an IP Address.
User opens the IP Address in a browser.
Web interface sends movement commands.
Robot navigates in real time.

Supported Commands:

Forward
Backward
Left
Right
Stop

Final Prototype

<img width="1280" height="960" alt="final" src="https://github.com/user-attachments/assets/4b85410b-4650-4353-bf71-b1bb1597f0fe" />


