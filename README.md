# Water Irrigation System

- This project is an automatic irrigation system that uses an ESP32 microcontroller to manage watering tasks. The system connects to a Node.js server via WebSockets for real-time communication and monitoring. This project represents the culmination of the author's final engineering work at Lublin University of Technology.
---

## Features
- **Automated Irrigation**: Controls water pumps and valves based on predefined logic.
- **WebSocket Communication**: Real-time data exchange between the server and ESP32 controller.
- **Node.js Backend**: Manages server-side operations and API endpoints.

---

## Prerequisites
- **Node.js**: Ensure that Node.js is installed on your machine. [Download Node.js](https://nodejs.org/)
- **npm**: Comes bundled with Node.js for managing dependencies.

---

## Installation and Setup

1. **Install Node.js**
   Download and install Node.js from the [official website](https://nodejs.org/).

2. **Install npm**
   npm is included with Node.js. Verify the installation by running:
   ```bash
   npm -v
   ```

3. **Clone the Repository**
   Clone this repository to your local machine:
   ```bash
   git clone <repository-url>
   ```

4. **Navigate to the API Directory**
   Change to the `water-irrigation-system-api` directory:
   ```bash
   cd water_irrigation_system_api
   ```

5. **Install Dependencies**
   Run the following command to install required packages:
   ```bash
   npm install
   ```

6. **Start the Server**
   Launch the server with the following command:
   ```bash
   node index.js
   ```

7. **Open the Application in Your Browser**
   Navigate to the following URL in your browser:
   ```
   http://localhost:8080
   ```

---

## Troubleshooting

- **Dependency Issues**: If you encounter issues with dependencies, try removing the `node_modules` folder and `package-lock.json` file, then run `npm install` again.
- **Port Conflicts**: Ensure no other applications are using port `8080`. If necessary, modify the `index.js` file to change the port.
- **WebSocket Connectivity**: Ensure that the ESP32 is configured to connect to the server via WebSocket.

---

## Notes

- This project is currently intended for local testing and development. Additional setup may be required for production use.
- Ensure your ESP32 device is properly configured and connected to the same network as the server.

---

## License
This project is open-source and licensed under the MIT License.

---

Enjoy using the Water Irrigation System API! 🌱

