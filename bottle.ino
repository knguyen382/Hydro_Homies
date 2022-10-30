#include <HCSR04.h>
#include <ArduinoBLE.h>



#define TRIGGER_PIN  0  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     1  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

long unsigned int Volume;
//float PI = 3.1415;
float radius = 4.5;
float height_water;
float height_sensor;

HCSR04 sonar(TRIGGER_PIN, ECHO_PIN);
BLEService hydro_homies("19B10000-E8F2-537E-4F6C-D104768A1214");

//NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  BLE.setLocalName("Hydro");
  BLE.setAdvertisedService(hydro_homies);

}

void loop() {
  //delay(200);                     // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.

  /*height_sensor = sonar.dist();
  Serial.print("Height of sensor = ");
  Serial.print(height_sensor); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println("cm");

  height_water = (9.5) - (height_sensor);
  Serial.print("Height of water = ");
  Serial.print(height_water); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println("cm");

  Serial.print("Volume: ");
  Serial.print(3.14 * radius * radius * height_water);
  Serial.println("mL");*/
  volume_calculation();
  Serial.println(Volume);
  
}

void volume_calculation()
{
  float water_level = 0;// store level in every step
  int read_value = 0; //read sensor reading in cm
  float average_water_level;
  int water_amount_in_ounce;
  for(int i=0; i<5; i++)
  { //take five reading
      read_value = sonar.dist();
      water_level = water_level + read_value;
        
  }
  average_water_level = 9.5 - water_level/5; //find average from five reading, 17 = botole height
  water_amount_in_ounce = int(3.14 * radius * radius * water_level);
  Volume = water_amount_in_ounce;
  delay(10);
}

