#include <Arduino_LSM9DS1.h>
#include <MadgwickAHRS.h>

Madgwick filter;

/*Accelerometer sample rate = 119.00 Hz
Gyroscope sample rate = 119.00 Hz
Magnetic field sample rate = 20.00 Hz*/
const float sampleRate = 20.0;   // Slowest sensor's sample rate
unsigned long microsPerReading = 1000000 / sampleRate;
unsigned long currMicros;
unsigned long prevMicros;

// magnetometer offset found after calibration script:
const float mag_hard_offset[3] = {-0.33875, 41.1578, 13.1989};   // hard iron offset
// const float mag_soft_distort[3][3] = {
//   {,,},
//   {,,},
//   {,,}
// }


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("IMU initialization failed");
    while (1);
  }

  filter.begin(sampleRate);

  prevMicros = micros();
}

void loop() {
  
  currMicros = micros();

  if((currMicros - prevMicros) >= microsPerReading){

    prevMicros = currMicros;  // update old micros for next loop

    float og_mx, og_my, og_mz;
    float post_hard_mx, post_hard_my, post_hard_mz;
    float ax, ay, az, gx, gy, gz, mx, my, mz;

    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable() && IMU.magneticFieldAvailable()) {
    
    // read data from accelerometer, gyroscope, magnetometer
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
    IMU.readMagneticField(og_mx, og_my, og_mz);

    // calibration correction for magnetometer (hopefully works lol)
    // hard-iron calibration:
    post_hard_mx = og_mx - mag_hard_offset[0];
    post_hard_my = og_my - mag_hard_offset[1];
    post_hard_mz = og_mz - mag_hard_offset[2];

    // soft-iron calibration (!DOESN'T WORK!)
    // mx = mag_soft_distort[0][0] * post_hard_mx + mag_soft_distort[0][1] * post_hard_my + mag_soft_distort[0][2] * post_hard_mz;
    // my = mag_soft_distort[1][0] * post_hard_mx + mag_soft_distort[1][1] * post_hard_my + mag_soft_distort[1][2] * post_hard_mz;
    // mz = mag_soft_distort[2][0] * post_hard_mx + mag_soft_distort[2][1] * post_hard_my + mag_soft_distort[2][2] * post_hard_mz;

    // madgwick requires gyroscope in radians/sec but LSM9S1 gives output in degrees/sec
    gx *= DEG_TO_RAD;
    gy *= DEG_TO_RAD;
    gz *= DEG_TO_RAD;

    // filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);
    filter.update(gx, gy, gz, ax, ay, az, post_hard_mx, post_hard_my, post_hard_mz);
    
    // euler angles (in degrees)
    float roll = filter.getRoll();
    float pitch = filter.getPitch();
    float yaw = filter.getYaw();

    // CSV format: roll, pitch, yaw
    Serial.print(roll); Serial.print(",");
    Serial.print(pitch); Serial.print(",");
    Serial.println(yaw);
   
    }
  }
}
