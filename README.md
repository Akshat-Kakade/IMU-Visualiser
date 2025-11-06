# Arduino IMU VPython Visualizer

A simple project to visualize the orientation of an LSM9DS1 IMU in 3D using Python and VPython.

The Arduino reads the IMU data, uses the Madgwick filter to get roll, pitch, and yaw, and streams it over serial. The Python script reads that serial data and rotates a 3D box to match.

![blevis(1)](https://github.com/user-attachments/assets/500df0a7-fd9d-44a0-bdc1-040a99f41e34)

## How it Works

There are two main parts:

### 1\. Arduino (`3dvis_arduino.ino`)

  * Runs on an Arduino with an onboard LSM9DS1 (like the Nano 33 BLE).
  * Includes the `Arduino_LSM9DS1` and `MadgwickAHRS` libraries.
  * Initializes the IMU and the Madgwick filter.
  * **Important:** It applies a hard-iron calibration offset to the magnetometer data. The values in the script are for *my* sensor. You MUST find your own, otherwise your yaw will drift like crazy.
  * In the main loop, it reads all 9-DOF (accel, gyro, mag).
  * It passes the calibrated data to the filter.
  * It prints the final `roll,pitch,yaw` angles as a comma-separated string to the Serial port.

### 2\. Python (`main.py`)

  * Uses `pyserial` to connect to the Arduino's serial port.
  * Uses `vpython` to create a simple 3D scene with a red box.
  * Continuously reads the serial port, line by line.
  * Parses the `roll,pitch,yaw` string into floats.
  * **Note:** The first reading it gets is used as an offset to "zero" the object's starting position (I was having lots of issues without this for some reason).
  * Applies the rotations to the 3D box.

## Setup

### Arduino

1.  Open `3dvis_arduino.ino` in the Arduino IDE.
2.  Install the libraries:
      * `Arduino_LSM9DS1`
      * `MadgwickAHRS` (by Arduino)
3.  **Calibrate your magnetometer\!** Use one of the many calibration sketches available (like the one from the `MadgwickAHRS` examples) to find your hard-iron offsets.
4.  Update the `mag_hard_offset` array in the sketch with your values.
5.  Upload the code to your board.

### Python

1.  Make sure you have Python 3.
2.  Install the required libraries:
    ```bash
    pip install pyserial vpython
    ```
3.  **Edit `main.py`:**
      * Find this line: `ser = serial.Serial('COM6', 115200)`
      * Change `'COM6'` to whatever serial port your Arduino is on (`/dev/ttyACM0` on Linux).

## How to Run

1.  After uploading the Arduino code, lay the board flat on your desk.
2.  Run the Python script:
    ```bash
    python main.py
    ```
3.  A VPython window should open showing a red box.
4.  Pick up and move the Arduino. The box should follow its orientation.

## Notes & Quirks

  * **Axis Swap:** In `main.py`, you'll see `pitch, yaw = yaw, pitch`. For some reason, the coordinate system from the filter didn't map directly to VPython's coordinate system. This swap makes it look correct.
  * **Startup Offset:** The Python script "zeros" itself on the first valid serial message it receives. If the visualization is "stuck" at a weird angle, just restart the Python script while the Arduino is laying flat.
  * **Rotation Order:** The final rotations are applied in Y-P-R order (Yaw, then Pitch, then Roll). This seemed to give the most intuitive result and avoid gimbal lock issues for this setup.
  * **Soft-Iron:** Soft-iron calibration is commented out in the Arduino file. It's more complex to calculate and hard-iron was good enough for this quick visualization.
