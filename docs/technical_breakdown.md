# Project Technical Breakdown

This document covers the *why* and *how* of the IMU visualizer project, going deeper than the main `README.md`.

## System Architecture

The setup is a simple two-part system:

- **Firmware (Arduino):** The "sensor" end. Its only job is to read the IMU, process the data using the Madgwick filter, and stream the resulting orientation (roll, pitch, yaw) as a CSV string over USB serial.
- **Visualizer (Python):** The "host" end. Its job is to listen to that serial port, parse the CSV string, and update a 3D VPython model in real-time.

**Data flow:**  
`IMU Hardware → Arduino_LSM9DS1 Lib → MadgwickAHRS Lib → Serial.println() → ser.readline() → VPython cube.rotate()`

## Part 1: Firmware (.ino) Deep Dive

### Libraries

- **Arduino_LSM9DS1.h**  
  The standard library for the on-board IMU. It handles all the low-level I2C communication to read the accelerometer, gyroscope, and magnetometer.
- **MadgwickAHRS.h**  
  This is the core of the sensor fusion:
  - A gyroscope is good for fast rotation but drifts over time.
  - An accelerometer is good for gravity (`pitch/roll`) but noisy (vibration).
  - A magnetometer is good for North (`yaw`) but easily disturbed (hard/soft iron).
  - The Madgwick filter (a type of AHRS) brilliantly fuses all three: it uses the gyro for primary rotation data but *corrects* its drift using the accel and mag data.

### Timing and Sample Rate (`loop()`)

The `loop()` function uses a non-blocking timer based on `micros()`:

```
if ((currMicros - prevMicros) >= microsPerReading) {
prevMicros = currMicros;
// ... do work ...
}
```

This is much better than using `delay()`, which would block the processor and could cause sensor readings to be missed.

In `3dvis_arduino.ino`, the `sampleRate` is 20 Hz.  
This is a bottleneck: it's set to 20 Hz because that's the fastest rate of the slowest sensor (the magnetometer). As a result, roll and pitch feel laggy, even though the gyro/accel can run much faster (119 Hz).

A better (but slightly more complex) approach is used in `faster3dvis.ino`:

- Run the main loop at the fastest sensor rate (119 Hz).
- On every loop, read the gyro and accel.
- Call `filter.updateIMU(gx, gy, gz, ax, ay, az)`. This updates roll and pitch at 119 Hz.
- Only when new magnetometer data is available, read it and call the full `filter.update(...)`. This corrects yaw drift at the slower 20 Hz, while roll/pitch remain fast.

### 🧲 Calibration: The Most Critical Part

The filter is only as good as the data you give it.  
The magnetometer is almost always wrong out of the box.

#### Hard-Iron Offset (`mag_hard_offset`)

- This is for static magnetic fields (e.g., a screw on the board). It adds a fixed offset to the mag readings (e.g., mx always reads +5 instead of 0).
- The calibration script finds these offsets.
- We fix it by simply subtracting the offset:  
  `post_hard_mx = og_mx - mag_hard_offset[0];`
- Without this, the filter gets a bad "North" reference, and the yaw will spin endlessly.

#### Soft-Iron Distortion (Commented Out)

- This is for distortions of the magnetic field (metal objects stretching the field).
- It requires a complex 3x3 transformation matrix to fix.
- For this project, the hard-iron fix was good enough.

## Part 2: Visualizer (`main.py`) Deep Dive

### Libraries

- **pyserial**  
  The standard for serial port communication. `ser.readline()` is perfect since the Arduino sends `\n` at the end of each `Serial.println()`.
- **vpython**  
  A simple-to-use 3D graphics library. `box()` creates a 3D object, and `rotate()` does the work.

### The "Zeroing" Offset

```
if roll_offset is None:
roll_offset = roll
pitch_offset = pitch
yaw_offset = yaw

roll -= roll_offset
pitch -= pitch_offset
yaw -= yaw_offset
```

This was a key fix.  
The Madgwick filter initializes its yaw based on "true North." For visualization, the goal is for the 3D box to be *flat* (0, 0, 0) relative to its starting orientation, regardless of North.

This code block acts as a "tare" or "zero" function.  
It grabs the first data packet, stores those angles as the offset, and subtracts that offset from all future readings.

### 🔄 The 3D Rotation Problem

3D rotations are a nightmare. You can't just keep applying relative rotations, or you'll get **gimbal lock**.

The correct way to apply Euler angles is to **reset** the object's orientation every single frame and then apply the new absolute rotation from scratch.

```
cube.axis = vector(1, 0, 0)
cube.up = vector(0, 1, 0)
```

The order of rotations matters.  
After much trial and error, this Y-P-R sequence (Yaw, then Pitch, then Roll) gave the most intuitive result:

yaw = z, pitch = y, roll = x
GOATS : ypr-yrp

```
cube.rotate(angle=yaw_rad, axis=vector(0, 0, 1))
cube.rotate(angle=pitch_rad, axis=vector(0, 1, 0))
cube.rotate(angle=roll_rad, axis=vector(1, 0, 0))
```

Finally, swapping `pitch, yaw = yaw, pitch` was a necessary fudge.  
The IMU's coordinate system didn't match VPython's, so this swap made the visualization match the physical movement.

## Future Improvements

- **Use Quaternions:**  
  The Madgwick filter can output quaternions directly. The Python script should read these instead. Quaternions are the mathematically "pure" way to represent 3D rotation and don't suffer from gimbal lock.
- **Soft-Iron Calibration:**  
  Implement the 3x3 matrix calibration for an even more stable yaw.
- **Auto-Reconnect:**  
  The Python script crashes if the COM port is unplugged. A more robust version would auto-scan for the port and attempt to reconnect.