#include <HCSR04.h>
#include <WiFi.h>
#include "time.h"

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Sensor's trigger and echo pin
#define TRIGGER_PIN  19  
#define ECHO_PIN     21  
#define BUZZER 13

int last_water_consumed_time;
float temp_volume;
float water_consumed;
float water_intake_remaining;
float daily_water_intake;

int i;
int k;
float duration[10], distance;
float duration_sum = 0;
float duration_avg;
float volume;
float compare[2];

// wifi stuff
const char* ssid = "abc";
const char* password = "6785702328";
String header;
WiFiServer server(80);

// web app stuff
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

HCSR04 sonar(TRIGGER_PIN, ECHO_PIN);

int get_time(char time_element);
void web_app(void *pvParameters);
void wifi(void * pvParameters);
float get_volume();
void waterIntakeCalculation(void *pvParameters);
void alarm();

void setup() {
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  Serial.begin(115200);
  // RTOS TASKS 

  // web app task
  xTaskCreatePinnedToCore(
  web_app
  ,  "web_app"   
  ,  5120  
  ,  NULL
  ,  2  
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);
  
  //wifi task
  xTaskCreatePinnedToCore(
  wifi
  ,  "wifi"   
  ,  4096  
  ,  NULL
  ,  1 
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
  waterIntakeCalculation
  ,  "volume"   
  ,  4096  
  ,  NULL
  ,  2  
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);

}

void loop() 
{
  //do nothing here, let RTOS handle its tasks
}

// web application fucntion
void web_app(void *pvParameters)
{
  (void) pvParameters;
  while(1)
  {  
    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) 
    {                             // If a new client connects,
      currentTime = millis();
      previousTime = currentTime;
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
        currentTime = millis();
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
              
              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              
              // buttons
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555;}</style></head>");
              
              // Web Page Heading
              client.println("<body><h1>Hydro Homies</h1>");
              
              // Select your gender
              client.println("<p>Select your gender: </p>");
              client.println("<p><a href=\"/M\"><button class=\"button\">MALE</button></a></p>");
              client.print("<p><a href=\"/F\"><button class=\"button\">FEMALE</button></a></p>");

              client.println("<p>Your recommended daily water intake is: </p>");
              client.print(daily_water_intake);
              client.print(" oz. ");

              client.println("<p>Water consumed: </p>");
              client.print(water_consumed);
              client.print(" oz. ");

              client.println("<p>Water remaining til goal: </p>");
              client.print(water_intake_remaining);
              client.print(" oz. ");
              client.println("</body></html>");
        
              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
          if (currentLine.endsWith("GET /M")) {
            daily_water_intake = 104;             
          }
          if (currentLine.endsWith("GET /F")) {
            daily_water_intake = 72;               
          }
        }
      }
      // Clear the header variable
      header = "";
      // Close the connection
      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    }
    vTaskDelay(100);
  }
}

// wifi function
void wifi(void *pvParameters)
{
  (void) pvParameters;
  while(1)
  {
    //if wifi is connected, delay task for 30s
    if(WiFi.status() == WL_CONNECTED)
    { 
      // check status every 30s
      vTaskDelay(30000/portTICK_PERIOD_MS);
      continue;
    }
    //if not, try to connect and start server
    else
    {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      // Print local IP address and start web server
      Serial.println("");
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      server.begin();
    }
    vTaskDelay(100);
  }
}

float get_volume()
{
    float volume;
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    
     if (i < sizeof(duration)/sizeof(float)) { //Takes 10 Samples of Duration
    duration [i] = pulseIn(ECHO_PIN, HIGH);
    i++;
  }
  else {  //Add all elements of Duration Array
    for (int j = 0; j < sizeof(duration)/sizeof(float); j++) {
      duration_sum = duration_sum + duration[j];      
    }
    duration_avg = duration_sum/(sizeof(duration)/sizeof(float));
    if (k < 2) {
      compare[k] = duration_avg*.0343/2;
      Serial.println(k);
      k++;
      if (compare[0] == compare[1]) {
        distance = compare[0];
        if (compare[0] == compare[1]) {
        distance = compare[0];
        if (distance < 4.0) {
          volume = 1000;
        }
        else if (distance >= 4.0 && distance < 6.0) {
          volume = 900;
        }
        else if (distance >= 6.0 && distance < 8.0) {
          volume = 800;
        }
        else if (distance >= 8.0 && distance < 9.0) {
          volume = 700;
        }
        else if (distance >= 9.0 && distance < 10.0) {
          volume = 600;
        }
        else if (distance >= 10.0 && distance < 12.0) {
          volume = 500;
        }
        else if (distance >= 12.0 && distance < 14.0) {
          volume = 400;
        }
        else if (distance >= 14.0 && distance < 16.0) {
          volume = 300;
        }
        else if (distance >= 16.0 && distance < 17.0) {
          volume = 200;
        }
        else if (distance >= 17.0 && distance < 18.0) {
          volume = 100;
        }
        else if (distance >= 18) {
          volume = 0;
        }
        Serial.println("");
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm");
      }
      if (k == 2) {
        k = 0;
      }
    }
    duration_sum = 0;
  }
    return volume;
}

int get_time(char time_element)
{
  time_t now = time(0);
  tm *ltm = localtime(&now);
  switch(time_element)
  {
    case ('h'):
      return ltm->tm_hour;
      break;
    case ('m'):
      return ltm->tm_min;
      break;
    case ('s'):
      return ltm->tm_sec;
      break;
  }
  return 0;
}

void waterIntakeCalculation(void * pvParameters)
{
  (void) pvParameters;
  while(1)
  {
    float current_volume = get_volume();
    int current_time = get_time('h');

    // some water consumed
    if(current_volume < temp_volume)
    {
      water_consumed += temp_volume - current_volume;
      last_water_consumed_time = get_time('h');
    }
    // alarm if no water consume for more than 2 hours
    if(get_time('h') - last_water_consumed_time >= 2 && last_water_consumed_time != 0)
    {
      alarm();
    }

    // calculate water remainting til hitting goals
    water_intake_remaining = daily_water_intake - water_consumed;

    // update temporary volume
    temp_volume = get_volume();

    if (get_time('h') == 0 && get_time('m') < 15) // new day, reset
    {
      water_intake_remaining = 0;
      last_water_consumed_time = 0;
    }
  }
  //run calculation every 15 min
  vTaskDelay(90000/portTICK_PERIOD_MS);
}

void alarm()
{
  for (int i = 0; i < 5; i ++)
  {  
    tone(BUZZER, 1000);
    delay(750);
    noTone(BUZZER);
    delay(750); 
  }
}
