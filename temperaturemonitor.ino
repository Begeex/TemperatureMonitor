/*
This program reads temperature and humidity data from RuuviTag and DHT-22
using an ESP32 and displays it on a Heltec 2.13 inch E-ind display.
The ESP32 measures data and updates display every 15 minutes.

The program uses following libraries:
DHT sensor library by Adafruit (1.4.6)
Heltec-E-Ink by HelTecAutomation (May.6.2020 version) I couldn't get much newer versions to work.
BLE library coming with ESP32 version 3.1.0

Left serial prints commented out for easier debugging in the future

Large arrays used in this code are stored in numbers.h

Screen pins -> ESP32
VCC -> 3.3V
GND -> GND    NOTE! GND not CND. Cheap boards might have this mislabelled
D/C -> D22
SDI -> D27
CS -> D18
CLK -> D5
BUSY -> D23

DHT-22 (module) -> ESP32
VCC -> 3.3V
GND -> GND
DAT -> D4


*/


#include "QYEG0213RWS800.h"
#include "numbers.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEAddress.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 4     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT22
#define LED 2
#define DHTpower 33

int TIME_TO_SLEEP = 60;                        //Time esp32 wil sleep in seconds (15min)

unsigned long long uS_TO_S_FACTOR = 1000000;  // Conversion factor for microseconds to seconds

DHT_Unified dht(DHTPIN, DHTTYPE);

int scanTime = 5;  // In seconds
BLEScan *pBLEScan;
String targetMAC = "fa:93:2b:06:ca:b2";  //MAC address of the wanted device
double temperature = 9999;    //Dummy values for variables
double humidity = 9999;
float temperature2 = 9999;
float humidity2 = 9999;
String data;  // Variable to store manufacturer data in hexadecimal format
String bin;
bool found = false;
int timesfailed = 0;


//Modified version library example class. I got help from code (not working anymore) from hackster.io
//https://www.hackster.io/amir-pournasserian/ble-weather-station-with-esp32-and-ruuvi-e8a68d
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks 
{
  void onResult(BLEAdvertisedDevice advertisedDevice) 
  {
    String deviceMAC = advertisedDevice.getAddress().toString().c_str();  //Get MAC address of the found device

    if (deviceMAC == targetMAC) //Check if MAC address is one of the wanted device
    {  
      //Serial.println("Target device found!");
      found = true;

      //Check if device has manufacturer data
      if (advertisedDevice.haveManufacturerData()) 
      {
        String manufacturerData = advertisedDevice.getManufacturerData();
        // Convert binary data to a readable hexadecimal format
        data = "";  // Clear previous data

        for (size_t i = 0; i < manufacturerData.length(); i++) 
        {
          char hexByte[3];
          sprintf(hexByte, "%02X", (unsigned char)manufacturerData[i]);  // Convert each byte to hex
          //Appending byte to string
          data += hexByte;
        }
        //Serial.println("Data Found.");
      } 

      else 
      {
        //Serial.println("No Manufacturer Data Found.");
      }

    }

  }
};

//Following function is borrowed from geeksforgeeks
//https://www.geeksforgeeks.org/program-binary-decimal-conversion/
int binaryToDecimal(String n)   //Function that changes binary to decimal numbers
{
    String num = n;
    int dec_value = 0;
    int base = 1;
    int len = num.length();

    for (int i = len - 1; i >= 0; i--) 
    {
        if (num[i] == '1')
            dec_value += base;
        base = base * 2;
    }
 
    return dec_value;
}

String hextobin(String input)   //Function that changes hexadecimal values to binary values
{
    String binary = "0000";
    String totalbin = "";

    int len = input.length();

    for (int i = 0; i < len; i++)   //Loop that goes through the input string
    {
        char currentChar = input.charAt(i);

        switch (currentChar)  //Using switch case to detect bianry equivalent for each hex number
        {
            case '0': binary = "0000"; break;
            case '1': binary = "0001"; break;
            case '2': binary = "0010"; break;
            case '3': binary = "0011"; break;
            case '4': binary = "0100"; break;
            case '5': binary = "0101"; break;
            case '6': binary = "0110"; break;
            case '7': binary = "0111"; break;
            case '8': binary = "1000"; break;
            case '9': binary = "1001"; break;
            case 'A': binary = "1010"; break;
            case 'B': binary = "1011"; break;
            case 'C': binary = "1100"; break;
            case 'D': binary = "1101"; break;
            case 'E': binary = "1110"; break;
            case 'F': binary = "1111"; break;
            default: 
                Serial.println("Error: Invalid hex character");
                return "";
        }

        totalbin += binary;
    }
    //Totalbin includes the whole hex value in binary
    return totalbin;
}

void updateNumberDisplay(float temp1, const int position[3][2]) //Function that inserts each number to the character array
{
    if (temp1 < 0)  //If temp1 is negative insert minus sign
    {
      //Going through a 13 x 3 rectangle and replaccing characters in "allinfo" with characters from "minus"
      for (int y = 0; y < 13; y++) 
        {
          for (int x = 0; x < 3; x++) 
          {
            gImage_allinfo[16 * (135 - 1 + y) + (7 - 1 + x)] = minus[y * 3 + x];
          }
        }
    }
    else  //If temp1 is positive insert blank
    {
      for (int y = 0; y < 13; y++) 
        {
            for (int x = 0; x < 3; x++) 
            {
                gImage_allinfo[16 * (135 - 1 + y) + (7 - 1 + x)] = null[y * 3 + x];
            }
        }
    }
    
    float temp1abs = abs(temp1);  //Taking the absolute value of number so we don't have to worry about minus sign
    int scaled = temp1abs*10;     //Scaling float to larger value to make it easier to print each number separately
    //Each temperature and humidity value has maximum of 3 digits. Here we are formatting all digits to 10 (empty)
    int digit1 = 10;
    int digit2 = 10;
    int digit3 = 10;

    //Using if and else statements to print the right amount of digits
    if ((temp1abs < 100) && (temp1abs >= 10)) 
    {
      digit1 = scaled / 100;
      digit2 = (scaled / 10) % 10;
      digit3 = scaled % 10;
    }
    else if (temp1abs < 10)
    {
      digit1 = 10;
      digit2 = scaled / 10;
      digit3 = scaled % 10;
    }
    else
    {
      digit1 = 10;
      digit2 = 10;
      digit3 = 10;
    }

    //Storing digits to an array to make looping easier
    int digits[3] = {digit1, digit2, digit3};

    //Loop through each digit and update the display
    for (int d = 0; d < 3; d++) 
    {
        int posX = position[d][0];
        int posY = position[d][1];

        //Drawing the digit to the character array
        for (int y = 0; y < 13; y++) //Y-axis
        {
            for (int x = 0; x < 3; x++) //X-axis
            {
              //Replacing characters of "allinfo" with characters from "numbers (x)""
                gImage_allinfo[16 * (posY - 1 + y) + (posX - 1 + x)] = numbers[digits[d]][y * 3 + x];
            }
        }
    }
}



void setup() 
{
  Serial.begin(115200);
  //Serial.println("Waking up");
  
  pinMode(LED,OUTPUT);
  pinMode(DHTpower,OUTPUT);
  
  //Powering on DHT-22
  digitalWrite(DHTpower,HIGH);

  //Flashing led at the beginning to see if ESP32 powers on and if it gets enough power
  delay(100);
  digitalWrite(LED,HIGH);
  delay(100);
  digitalWrite(LED,LOW);
  delay(100);
  dht.begin();
  epd213.EPD_HW_Init(); //Electronic paper initialization
  //Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // Create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // Active scan uses more power, but gets results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // Less or equal to setInterval value
  delay(1000);


  for (int x = 0; x < 5; x++) //Using a loop to try and read data over BLE 5 times. after that continue.
  {
    BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);

    //Serial.println("Scan done!");

    if (found == true) //data was found
    {
      //Serial.print("Collected Manufacturer Data:");
      //Serial.println(data);

      //Temperature data is located in manufacturer data (6, 10)
      String hexin = (data.substring(6, 10));
      //Calling hextobin to convert data
      String bin = hextobin(hexin);
      //Serial.println(bin);

      //Checking if temperature is negative
      if (bin.charAt(0)=='1')
      {
          //Serial.print("Negative: ");
          String negbin = bin.substring(1, bin.length());
          //Serial.print("Binary");
          //Serial.println(negbin);
          //Saving temperature data in decimal format
          temperature = -binaryToDecimal(bin)*0.005 + (binaryToDecimal(negbin)*0.005)*2;
          //Serial.println(temperature);

      }

      else
      {
          //Serial.print("Positive: ");
          //Saving temperature data in decimal format
          temperature = binaryToDecimal(bin)*0.005;
          //Serial.println(temperature);
      }

      //Humidity data is located in manufacturer data (10, 14)
      String hexin2 = (data.substring(10, 14));
      //Converting hex to bin
      String bin2 = hextobin(hexin2);
      //Serial.print("Humidity1: ");
      //Saving humidity data in decimal format
      humidity = binaryToDecimal(bin2)*0.0025;
      //Serial.println(humidity);

      found = false;
      break;
    } 
    
    else    //Data was not found
    {
      //Serial.println("No Manufacturer Data collected yet.");
      if (timesfailed > 4) //If failed to find data in 5 scans use blank values (9999) for temperature and humidity
      {
        //Serial.println("----------No devices detected in 5 scans----------");
        temperature = 9999;
        humidity = 9999;
        break;
      }
      else
      {
        timesfailed = timesfailed + 1;
      }

    }
    delay(1000);
  }
  timesfailed = 0;

  pBLEScan->clearResults();  // Delete results from BLEScan buffer to release memory

  //I figured the best way to get the info where each number is supposed to go is to store beginning positions to arrays
  const int position1[3][2] = 
  {
    {7, 24},  //Beginning position for the first digit
    {7, 43}, // Beginning position for the second digit
    {7, 70}  // Beginning position for the third digit
  };
  const int position2[3][2] = 
  {
    {7, 149},
    {7, 170},
    {7, 195}
  };
  const int position3[3][2] = 
  {
    {12, 31},
    {12, 50},
    {12, 70}
  };
  const int position4[3][2] = 
  {
    {12, 156},
    {12, 175},
    {12, 195}
  };

  // Getting temperature and humidity data from DHT-22
  // Following code is the same as in the library example
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) 
  {
    temperature2 = 9999;
  }
  else 
  {
    temperature2 = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) 
  {
    humidity2 = 9999;
  }
  else 
  {
    humidity2 = event.relative_humidity;
  }

  //Checking values from DHT-22
  //Serial.print("Humidity2: ");
  //Serial.println(humidity2);
  //Serial.print("Temperature2: ");
  //Serial.println(temperature2);

  //Calling function to update data to display
  //NOTE!! temperature is being updated last because in this use casi it's the only value that can be negative
  updateNumberDisplay(temperature2, position1);
  updateNumberDisplay(humidity2/10, position3);
  updateNumberDisplay(humidity/10, position4);
  updateNumberDisplay(temperature, position2);

  epd213.EPD_ALL_image(gImage_allinfo,gImage_213red); //Refresh the picture in full screen

  
  epd213.EPD_DeepSleep(); //Display enters deep sleep

  //Shutting down DHT-22
  digitalWrite(DHTpower, LOW);

  delay(1000);

  //Serial.println("Going to sleep now"); 
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);  // Setting the timer to wake up

  Serial.flush();           // Waits for the transmission of outgoing serial data to complete.

  esp_deep_sleep_start();  //ESP32 goes to deep sleep

}

void loop() {
  //Nothing here
}
