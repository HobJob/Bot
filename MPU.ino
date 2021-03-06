
#define PI 3.14159265358979323846264338327950288
#define numberOftimesCalibrated 2000

//Declaring some global variables
int gyro_x, gyro_y, gyro_z;
long acc_x, acc_y, acc_z, acc_total_vector;
int temperature;
long gyro_x_cal, gyro_y_cal, gyro_z_cal;
float angle_pitch, angle_roll;
boolean set_gyro_angles;
float angle_roll_acc, angle_pitch_acc;
float angle_pitch_output, angle_roll_output;

//Constants for the MPU-6050
float calcRaw;
float useRaw;
float ToRadians;



void setup_mpu_6050_registers(){
  Serial.println("Starting MPU-6050");
  //Activate the MPU-6050
  Wire.beginTransmission(0x68);                                                 //Start communicating with the MPU-6050
  Wire.write(0x6B);                                                             //Send the requested starting register
  Wire.write(0x00);                                                             //Set the requested starting register
  Wire.endTransmission();                                                       //End the transmission
  //Configure the accelerometer (+/-8g)
  Wire.beginTransmission(0x68);                                                 //Start communicating with the MPU-6050
  Wire.write(0x1C);                                                             //Send the requested starting register
  Wire.write(0x10);                                                              //Set the requested starting register
  Wire.endTransmission();                                                        //End the transmission
  //Configure the gyro (500dps full scale)
  Wire.beginTransmission(0x68);                                                  //Start communicating with the MPU-6050
  Wire.write(0x1B);                                                              //Send the requested starting register
  Wire.write(0x08);                                                              //Set the requested starting register
  Wire.endTransmission();                                                        //End the transmission

  //Calibrating gyro

  for (int cal_int = 0; cal_int < numberOftimesCalibrated ; cal_int ++){                  //Run this code 2000 times
    read_mpu_6050_data();                                                       //Read the raw acc and gyro data from the MPU-6050
    gyro_x_cal += gyro_x;                                                       //Add the gyro x-axis offset to the gyro_x_cal variable
    gyro_y_cal += gyro_y;                                                       //Add the gyro y-axis offset to the gyro_y_cal variable
    gyro_z_cal += gyro_z;                                                        //Add the gyro z-axis offset to the gyro_z_cal variable
    delay(CLOCK/1000);
  }
  gyro_x_cal /= numberOftimesCalibrated;                                                  //Divide the gyro_x_cal variable by 2000 to get the avarage offset
  gyro_y_cal /= numberOftimesCalibrated;                                                  //Divide the gyro_y_cal variable by 2000 to get the avarage offset
  gyro_z_cal /= numberOftimesCalibrated;                                                  //Divide the gyro_z_cal variable by 2000 to get the avarage offset

  calculateConstantsGyro();
  Serial.println("MPU-6050 Calibrated!");
}


void CalcGyro(int DEBUG_Log){
  read_mpu_6050_data();                                                //Read the raw acc and gyro data from the MPU-6050
  gyro_x -= gyro_x_cal;                                                //Subtract the offset calibration value from the raw gyro_x value
  gyro_y -= gyro_y_cal;                                                //Subtract the offset calibration value from the raw gyro_y value
  gyro_z -= gyro_z_cal;                                                //Subtract the offset calibration value from the raw gyro_z value

  //Gyro angle calculations
  //0.0000611 = 1 / (250Hz * 65.5)

  angle_pitch += (float)gyro_x * calcRaw;                                   //Calculate the traveled pitch angle and add this to the angle_pitch variable
  angle_roll += (float)gyro_y * calcRaw;                                    //Calculate the traveled roll angle and add this to the angle_roll variable

  //0.1066 = 0.0000611 * (3.142(PI) / 180degr)   The Arduino sin function is in radians
  angle_pitch += angle_roll * sin(gyro_z * useRaw);               //If the IMU has yawed transfer the roll angle to the pitch angel
  angle_roll -= angle_pitch * sin(gyro_z * useRaw);               //If the IMU has yawed transfer the pitch angle to the roll angel

  //Accelerometer angle calculations
  acc_total_vector = sqrt((acc_x*acc_x)+(acc_y*acc_y)+(acc_z*acc_z));  //Calculate the total accelerometer vector
  //57.296 = 1 / (3.142 / 180) The Arduino asin function is in radians
  angle_pitch_acc = asin((float)acc_y/acc_total_vector)*  ToRadians;       //Calculate the pitch angle
  angle_roll_acc = asin((float)acc_x/acc_total_vector)* -ToRadians;       //Calculate the roll angle

  //Place the MPU-6050 spirit level and note the values in the following two lines for calibration
  angle_pitch_acc -= 0.0;                                              //Accelerometer calibration value for pitch
  angle_roll_acc -= 0.0;                                               //Accelerometer calibration value for roll

  if(set_gyro_angles){                                                 //If the IMU is already started
    angle_pitch = angle_pitch * 0.9996 + angle_pitch_acc * 0.0004;     //Correct the drift of the gyro pitch angle with the accelerometer pitch angle
    angle_roll = angle_roll * 0.9996 + angle_roll_acc * 0.0004;        //Correct the drift of the gyro roll angle with the accelerometer roll angle
  }
  else{                                                                //At first start
    angle_pitch = angle_pitch_acc;                                     //Set the gyro pitch angle equal to the accelerometer pitch angle
    angle_roll = angle_roll_acc;                                       //Set the gyro roll angle equal to the accelerometer roll angle
    set_gyro_angles = true;                                            //Set the IMU started flag
  }

  //To dampen the pitch and roll angles a complementary filter is used,
  angle_pitch_output = angle_pitch_output * 0.8 + angle_pitch * 0.2;   //Take 90% of the output pitch value and add 10% of the raw pitch value
  angle_roll_output = angle_roll_output * 0.8 + angle_roll * 0.2;      //Take 90% of the output roll value and add 10% of the raw roll value
  if(DEBUG_Log)showValues();
  }

void read_mpu_6050_data(){                                             //Subroutine for reading the raw gyro and accelerometer data
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x3B);                                                    //Send the requested starting register
  Wire.endTransmission();                                              //End the transmission
  Wire.requestFrom(0x68,14);                                           //Request 14 bytes from the MPU-6050
  while(Wire.available() < 14);                                        //Wait until all the bytes are received
  acc_x = Wire.read()<<8|Wire.read();                                  //Add the low and high byte to the acc_x variable
  acc_y = Wire.read()<<8|Wire.read();                                  //Add the low and high byte to the acc_y variable
  acc_z = Wire.read()<<8|Wire.read();                                  //Add the low and high byte to the acc_z variable
  temperature = Wire.read()<<8|Wire.read();                            //Add the low and high byte to the temperature variable
  gyro_x = Wire.read()<<8|Wire.read();                                 //Add the low and high byte to the gyro_x variable
  gyro_y = Wire.read()<<8|Wire.read();                                 //Add the low and high byte to the gyro_y variable
  gyro_z = Wire.read()<<8|Wire.read();                                 //Add the low and high byte to the gyro_z variable

  }


  void calculateConstantsGyro(){
    ToRadians = 1.0/(PI/180);
    calcRaw = 1.0 / ((1.0/(CLOCK / 1000000.0)) * 65.5);
    useRaw = calcRaw * (1.0/ToRadians);
  }

void showValues(){
  Serial.print("Pitch: ");Serial.print(angle_pitch_output);Serial.print(" Roll: ");Serial.println(angle_roll_output);
}
