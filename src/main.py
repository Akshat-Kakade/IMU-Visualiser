import math
import serial
from vpython import box, vector, rate, color, radians

try:
    # open the serial port
    ser = serial.Serial('COM6', 115200)
except serial.SerialException as e:
    print(f"Error: Could not open serial port")
    print(e)
    exit()

# create the 3D object
cube = box(length=1, height=0.2, width=0.5, color=color.red)
# add an axis indicator to clearly show the orientation
cube_axis_indicator = box(pos=vector(0.5, 0, 0), length=1, height=0.05, width=0.05, color=color.blue)
cube_axis_indicator.parent = cube

# calibration offsets (to avoid shitty orientation at bootup)
roll_offset = None
pitch_offset = None
yaw_offset = None

while True:
    rate(50)

    try:
        line = ser.readline().decode('utf-8').strip()
        if not line:
            continue

        roll, pitch, yaw = [float(x) for x in line.split(',')]

        # if offset is none then simulation just started so set as initial
        if roll_offset is None:
            roll_offset = roll
            pitch_offset = pitch
            yaw_offset = yaw

        # apply offsets UwU
        roll -= roll_offset
        pitch -= pitch_offset
        yaw -= yaw_offset

        # pitch and yaw interchange for some weird reason
        pitch, yaw = yaw, pitch

        # VPython requires angles in radians for rotation functions
        yaw_rad = radians(yaw)
        pitch_rad = radians(pitch)
        roll_rad = radians(roll)
        
        # reset object orientation before new rotation
        cube.axis = vector(1, 0, 0)
        cube.up = vector(0, 1, 0)

        # yaw = z, pitch = y, roll = x
        # GOATS : ypr-yrp 
        # Good Roll, shit Yaw : ryp-rpy
        # Meh: pry-pyr
        cube.rotate(angle=yaw_rad, axis=vector(0, 0, 1))
        cube.rotate(angle=pitch_rad, axis=vector(0, 1, 0))
        cube.rotate(angle=roll_rad, axis=vector(1, 0, 0))
        
        
    except ValueError:
        # error from shitty data
        continue
    except serial.SerialException as e:
        print(f"Serial port error: {e}")
        ser.close()
        break