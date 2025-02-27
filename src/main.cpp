#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include "Config.h"


Servo servo;

float POS_ON = 150;
float POS_OFF = 0;

struct t_val
{
    float price;
    uint8_t price_level;
};
t_val time_values[48];

float min_price = 1000;
float max_price = 0;
float avg_price = 0;

bool state = false;

uint8_t get_price_level(String price_level);
void timeAvailable(struct timeval *t);

void turn_servo_on();
void turn_servo_off();



bool connectToWiFi()
{
    Serial.println("Connecting to WiFi..."); // Wait for WiFi connection
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.config(IPAddress(192, 168, 178, 140), IPAddress(192, 168, 178, 100), IPAddress(255, 255, 255, 0), IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int connection_counter = 0;
    while (connection_counter < 20)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            break;
        }
        delay(500);
        Serial.print(".");
        connection_counter++;
    }
    if (connection_counter >= 20)
    {
        Serial.println("WiFi connection failed");
        return false;
    }
 
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    sntp_set_time_sync_notification_cb(timeAvailable);
    configTzTime(timeZone, ntpServer1);
    time_t timeNow = time(NULL); // Current time (Epoch) as time_t (seconds since 01.01.1970)
    struct tm tmNow;
    localtime_r(&timeNow, &tmNow); // Transfer time to tm with the correct time zone
    if (getLocalTime(&tmNow))
    {
        Serial.println("Time is set");
    }
    return true;
}

String fetchTibberPrices(String query)
{
    Serial.println("Fetching Tibber prices");
    WiFiClientSecure client; // HTTPS !!!
    client.setInsecure(); // the magic line, use with caution
    client.setCertificate(tibberSSLfingerprint);
    
    if (!client.connect(tibberApi, tibberPort))
    {
        Serial.println("Connection failed");
        return "";
    }

    //Serial.println("Sending query...");
    //Serial.println(query);
    
    client.println("POST /v1-beta/gql HTTP/1.1");
    client.println("Host: api.tibber.com");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.print("Authorization: Bearer ");
    client.println(tibberToken);
    client.print("Content-Length: ");
    client.println(query.length());
    client.println();
    client.println(query);

    while (client.connected())
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
            break;
        }
    }
    
    String response = "";
    while (client.available())
    {
        response += (char)client.read();
    }

    client.stop();
    return response;
}



void unpack_to_buffer(String response)
{
    JsonDocument doc;
    
    response.trim();
    //Serial.println(response);

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    for (int i = 0; i < 24; i++)
    {
        time_values[i].price = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["today"][i]["total"];
        String price_level = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["today"][i]["level"];
        time_values[i].price_level = get_price_level(price_level);
        
    }
    
    bool tomorrow_empty = false;
    for (int i = 0; i < 24; i++)
    {
        time_values[i + 24].price = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["tomorrow"][i]["total"];
        String price_level = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["tomorrow"][i]["level"];
        time_values[i + 24].price_level = get_price_level(price_level);

        if (time_values[i + 24].price_level == 5)
        {
            tomorrow_empty = true;
            break; 
        }  
    }
    


    min_price = 1000;
    max_price = 0;
    avg_price = 0;
    for (int i = 0; i < (tomorrow_empty ? 24 : 48); i++)
    {
        // Print the value 
        /*
        Serial.print(i);
        Serial.print(" : ");
        Serial.print(time_values[i].price);
        Serial.print(" : ");
        Serial.println(time_values[i].price_level);
        */
        
        
        // calculate min, max and avg price
        if (time_values[i].price < min_price)
        {
            min_price = time_values[i].price;
        }
        if (time_values[i].price > max_price)
        {
            max_price = time_values[i].price;
        }
        avg_price += time_values[i].price;
    }
    avg_price = avg_price / (tomorrow_empty ? 24 : 48);

    Serial.print("Min: ");
    Serial.println(min_price);
    Serial.print("Max: ");
    Serial.println(max_price);
    Serial.print("Avg: ");
    Serial.println(avg_price);
}

void evaluate_buffer()
{
    time_t timeNow = time(NULL);
    struct tm tmNow;
    localtime_r(&timeNow, &tmNow);
    int current_hour = tmNow.tm_hour;
    float current_price = time_values[current_hour].price;

    if (current_price < (avg_price - 0.01) && state == false)
    {
        servo.attach(servoPin); 
        delay(100);
        turn_servo_on();
        delay(200);
        servo.detach();
        
        state = true;
        digitalWrite(BUILTIN_LED, HIGH);
    }
    
    else if (current_price > (avg_price - 0.01) && state == true)
    {
        servo.attach(servoPin); 
        delay(100);
        turn_servo_off();
        delay(200);
        servo.detach();
        
        state = false;
        digitalWrite(BUILTIN_LED, LOW);
        
    }
    
    Serial.print("Current state: ");
    Serial.println(state);
}


void setup()
{
    
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, LOW);
    

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    Serial.begin(115200);

    servo.setPeriodHertz(50); // Standard 50hz servo
    
    servo.attach(servoPin); 

    servo.write(POS_OFF);
    delay(5000);
    
    servo.detach();
    delay(5000);
}

uint64_t last_time_fetch = 0;
uint64_t last_time_evaluate = 0;
void loop()
{
    if (millis() - last_time_fetch > fetch_interval || last_time_fetch == 0)
    {
        if (connectToWiFi())
        {
            delay(500);
            String respone = fetchTibberPrices(tibberQuerry);
            unpack_to_buffer(respone);
        }

        
        WiFi.disconnect();
        last_time_fetch = millis();
    }
    if (millis() - last_time_evaluate > evaluate_interval || last_time_evaluate == 0)
    {
        evaluate_buffer();
        last_time_evaluate = millis();
    }
    delay(1000);
}











uint8_t get_price_level(String price_level)
{
    if (price_level == "VERY_CHEAP")
    {
        return 0;
    }
    else if (price_level == "CHEAP")
    {
        return 1;
    }
    else if (price_level == "NORMAL")
    {
        return 2;
    }
    else if (price_level == "EXPENSIVE")
    {
        return 3;
    }
    else if (price_level == "VERY_EXPENSIVE")
    {
        return 4;
    }
    else
    {
        return 5;
    }
}

void timeAvailable(struct timeval *t)
{
    Serial.println("[WiFi]: Got time adjustment from NTP!");
    // rtc.hwClockWrite();
}

void turn_servo_on()
{
    servo.attach(servoPin); 
    delay(100);
    if(POS_ON > POS_OFF){
        for (int i = POS_OFF; i < POS_ON; i++)
        {
            servo.write(i);
            delay(10);
        }
    }
    else{
        for (int i = POS_OFF; i > POS_ON; i--)
        {
            servo.write(i);
            delay(10);
        }
    }
    servo.write(POS_ON);
    delay(1000);
    servo.detach();
}

void turn_servo_off()
{
    servo.attach(servoPin); 
    delay(100);
    if(POS_ON > POS_OFF){
        for (int i = POS_ON; i > POS_OFF; i--)
        {
            servo.write(i);
            delay(10);
        }
    }
    else{
        for (int i = POS_ON; i < POS_OFF; i++)
        {
            servo.write(i);
            delay(10);
        }
    }
    servo.write(POS_OFF);
    delay(1000);
    servo.detach();
}