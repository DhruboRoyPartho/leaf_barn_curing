// Version 1 er sensor duita ke ultay deya ache

// Project: Leaf Barn Curing
// Code Author: Dhrubo Roy Partho
// Date: 02/05/2024
// Version: 2.0v



// relay pins in #define
// Temp control in temp_control()
// time set in time_definer()
// 


#include <OneWire.h>
#include <DallasTemperature.h>
// #include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

// millis() variable for reset
extern volatile unsigned long timer0_millis;
// unsigned long new_value = 1000;

// Pins
#define temp_pin1 6
#define temp_pin2 5
#define dam_relay1 3
#define dam_relay2 4
#define blo_relay 2
#define btn_1 A0
#define btn_2 A1
#define btn_3 A2
#define btn_4 A3
#define cs_pin 10


// Memory save time delay
const int mem_delay = 10000;


// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(temp_pin1);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);


// Time counter
unsigned long curr_time = 0;
unsigned long pre_time = 0;
unsigned long temp_sense_pre_time = 0;
unsigned long hour = 0, min = 0, sec = 0, pre_h, pre_m, pre_sec;
unsigned long defined_time = 0;
unsigned long start_time = 0;
unsigned long end_time = 0;
unsigned long mem_pre_time = 0;


// Global variables
int numberOfDevices; // Number of temperature devices DS18B20
DeviceAddress tempDeviceAddress;  // For DS18B20

float tempC[2] = {};
float tempF[2] = {};

volatile bool state_1 = false;
volatile bool state_2 = false;
volatile bool state_3 = false;
volatile bool state_4 = false;

byte selected_phase = 0;
byte pre_selected_phase = 0;
bool flag = false;

String log_string[3] = {};



// Phase temperature data
const byte phase[4][14][2] = {{{95, 92}, {96, 93}, {98, 94}, {99, 95}, {100, 96}},
                              {{100, 96}, {102, 96}, {104, 97}, {106, 97}, {108, 98}, {110, 98}, {112, 98}, {114, 99}, {116, 99}, {118, 100}, {120, 100}},
                              {{120, 100}, {122, 100}, {124, 101}, {126, 101}, {128, 102}, {130, 102}, {132, 102}, {134, 103}, {136, 103}, {138, 104}, {140, 104}, {142, 104}, {144, 105}, {145, 105}},
                              {{145, 105}, {147, 106}, {151, 107}, {153, 107}, {155, 108}, {157, 108}, {159, 109}, {161, 109}, {163, 110}, {163, 110}, {165, 110}}};

// const byte phase_duration_h[4] = {4, 10, 14, 10};
// const byte phase_duration_m[4] = {0, 0, 30, 0};

byte loader_icon_counter = 0;

// File log_file;
File myFile;

// volatile bool reset_pre_state = false;


void setup(void)
{
  // Start serial communication for debugging purposes
  Serial.begin(9600);

  // Pin Change Interruption
    PCICR |= (1<<PCIE1);
    PCMSK1 |= (1<<PCINT8);
    PCMSK1 |= (1<<PCINT9);
    PCMSK1 |= (1<<PCINT10);
    PCMSK1 |= (1<<PCINT11);

  // DS18B20 initialization
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();

  // LCD initialization
  lcd.init();
  lcd.clear();
  lcd.backlight();

  lcd.print("Staring...");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Checking components");
  delay(1000);

  // Pin modes
  pinMode(dam_relay1, OUTPUT);
  pinMode(dam_relay2, OUTPUT);
  pinMode(blo_relay, OUTPUT);

  pinMode(btn_1, INPUT_PULLUP);
  pinMode(btn_2, INPUT_PULLUP);
  pinMode(btn_3, INPUT_PULLUP);
  pinMode(btn_4, INPUT_PULLUP);

    lcd.clear();

    lcd.print("Memory testing...");
    lcd.setCursor(0, 1);

    memory_check();

    // check read file
    if(SD.exists("log.txt")){
        memory_data_read();
    }

    blower_start();

    delay(1000);
}

// Interruption service routine
ISR(PCINT1_vect){

    if(digitalRead(btn_1)){
        state_1 = true;
        state_2 = false;
        state_3 = false;
        state_4 = false;
    }

    if(digitalRead(btn_2)){
        state_1 = false;
        state_2 = true;
        state_3 = false;
        state_4 = false;
    }

    if(digitalRead(btn_3)){
        state_1 = false;
        state_2 = false;
        state_3 = true;
        state_4 = false;
    }

    if(digitalRead(btn_4)){
        state_1 = false;
        state_2 = false;
        state_3 = false;
        state_4 = true;
    }
}


// Reset function
void (*resetFunc)(void) = 0;


void loop(void){
    // Serial.println("Running1");
  curr_time = millis();
//   setMillis(curr_time);
  time_calculator();

//   Serial.println("Running2");
  button_process();
  
  // Time and phase
  phase_decision();

  temp_measure();

  // temperature control
  temp_control();
 
  if(curr_time - pre_time > 3000){
    pre_time = curr_time;

    display();
  }

  if(selected_phase >= 1 && selected_phase <= 4 && (millis() - mem_pre_time) > mem_delay){      // 10 sec delay for memory writing
    memory_data_write();
    mem_pre_time = millis();
  }
}

// Memory check
void memory_check(){
    if(!SD.begin(cs_pin)){
        lcd.print("Memory error");
        return;
    }
    lcd.print("Memory ok");
}

// memory file existance
// bool memory_data_check(){
//     if(SD.exists("log.txt")){
//         Serial.println("Data exists");
//         return true;
//     }
//     else{
//         Serial.println("Not exists");
//         return false;
//     }
// }


// memory data read
void memory_data_read(){
    Serial.println("data read called");
    myFile = SD.open("log.txt", FILE_READ);
    if(myFile){
        // Read data
        byte i = 0;
        while(myFile.available()){
            log_string[i] = myFile.readStringUntil('\n');
            Serial.println(log_string[i]);
            i++; 
        }

        curr_time = convert_string_to_int(log_string[0]);
        defined_time = convert_string_to_int(log_string[1]);
        selected_phase = (int)convert_string_to_int(log_string[2]);

        setMillis(curr_time);

        // time conversion after read
        unsigned long currentMillis = curr_time;
        unsigned long seconds = currentMillis / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        unsigned long days = hours / 24;
        currentMillis %= 1000;
        seconds %= 60;
        minutes %= 60;
        hours %= 24;

        hour = hours;
        min = minutes;
        sec = seconds;

        // close file
        myFile.close();
    }
    else{
        lcd.print("File open error.");
    }

    Serial.print("Function over");

}

// memory data write
void memory_data_write(){
    if(SD.exists("log.txt"))
        SD.remove("log.txt");
    myFile = SD.open("log.txt", FILE_WRITE);
    if(myFile){
        myFile.println(curr_time);
        myFile.println(defined_time);
        myFile.println(selected_phase);
        myFile.close();
    }
    else{
        lcd.clear();
        lcd.print("Memory isn't writing");
        while(1){
            button_process();
        }
    }
}

unsigned long convert_string_to_int(String s){
    Serial.print("Length: ");
    // Serial.println(s.length());
    unsigned long num = 0;
    unsigned long j = 1;
    for(int i=s.length()-1;i>0;i--){
        // Serial.println(s[i-1] - '0');
        num += (s[i-1] - '0') * j;
        j *= 10;
    }
    return num;
}

void blower_start(){
    digitalWrite(blo_relay, HIGH);
}
void blower_stop(){
    digitalWrite(blo_relay, LOW);
}
void damper_open(){
    digitalWrite(dam_relay1, LOW);
    digitalWrite(dam_relay2, HIGH);
}
void damper_close(){
    digitalWrite(dam_relay2, LOW);
    digitalWrite(dam_relay1, HIGH);
}


void temp_control(){
    if((byte)tempF[0] < phase[selected_phase - 1][hour][0]){
        // Blower start
        blower_start();
    }
    else{
        // Blower stop
        blower_stop();
    }

    if((byte)tempF[1] > phase[selected_phase - 1][hour][1]){
        // damper open
        damper_open();
    }
    else{
        // damper close
        damper_close();
    }
}

void phase_decision(){
    if(curr_time > defined_time){
        pre_selected_phase = selected_phase;
        selected_phase = 0;
        if(flag == true)
            wait_for_next_phase();
        return;
    }
    return;
}

void wait_for_next_phase(){
    unsigned long pre_wait_time = 0;
    unsigned long pre_caution = 0;
    setMillis(0);
    while(selected_phase == 0){
        button_process();       // button process addition
        if(millis() - pre_caution > 2000){
            lcd.clear();
            lcd.print("Phase ");
            lcd.print(pre_selected_phase);
            lcd.print(" over.");
            lcd.setCursor(0, 1);
            lcd.print("Waitng for 10sec");
            pre_caution = millis();
        }
        if(millis() > 10000){           // Waiting for 10 sec to forward next phase
            if(pre_selected_phase + 1 > 4){
                lcd.clear();
                lcd.print("All Phase Covered");
                lcd.setCursor(0, 1);
                lcd.print("Please select again.");
                long unsigned pre_phase_time = millis();

                while(millis() - pre_phase_time < 10000){
                    button_process();
                    pre_phase_time = millis();
                }

                if(SD.exists("log.txt")){
                    SD.remove("log.txt");
                }
                resetFunc();
                // while(1);
            }
            else{
                selected_phase = pre_selected_phase + 1;
                time_definer();
                reset_all_time();
            }
        }
    }
}


void time_definer(){
    if(selected_phase == 1){
        defined_time = 1.44e+7;      // milliseconds
    }
    else if(selected_phase == 2){
        defined_time = 3.6e+7;
    }
    else if(selected_phase == 3){
        defined_time = 4.5e+7;
    }
    else if(selected_phase == 4){
        defined_time = 3.6e+7;
    }
    else{
        defined_time = 0;
    }
}


// Time calcultaor
void time_calculator(){ 
    if(curr_time - pre_sec >= 1000){
        ++sec;
        if(sec >= 60){
            ++min;
            if(min >= 60){
                ++hour;
                min = 0;
            }
            sec = 0;
        }
        pre_sec = curr_time;
    }
}


// Button process function
void button_process(){
    if(state_1 == true){
        selected_phase = 1;
        
        time_definer();

        reset_all_time();

        memory_data_write();
        display();
        state_1 = false;
        
        flag = true;
    }
     if(state_2 == true){
        selected_phase = 2;
        
        time_definer();

        reset_all_time();
        memory_data_write();
        display();

        state_2 = false;

        flag = true;
    }
     if(state_3 == true){
        
        selected_phase = 3;
        // End time
        time_definer();

        reset_all_time();

        memory_data_write();
        display();

        state_3 = false;

        flag = true;
    }
    if(state_4 == true){
        
        selected_phase = 4;
        // End time
        time_definer();

        reset_all_time();

        memory_data_write();
        display();

        state_4 = false;

        flag = true;
    }
}

// Reset all time
void reset_all_time(){
    curr_time = 0;
    pre_time = 0;
    temp_sense_pre_time = 0;
    hour = 0, min = 0, sec = 0, pre_h = 0, pre_m = 0, pre_sec = 0;
    setMillis(0);
}


// Temperature sensing
void temp_measure(){
  if(curr_time - temp_sense_pre_time > 3000){
    // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
    sensors.requestTemperatures();

    int idx = 0;

    // Loop through each device, print out temperature data
    for(int i=numberOfDevices-1;i>=0; i--) {
        // Search the wire for address
        if(sensors.getAddress(tempDeviceAddress, i)){
          tempC[idx] = sensors.getTempC(tempDeviceAddress);
          tempF[idx] = DallasTemperature::toFahrenheit(tempC[i]);
        //   Serial.println(tempF[i]);    //DallasTemperature::toFahrenheit(tempC)   // Converts tempC to Fahrenheit
        } 	
        idx++;
    }

    // display();
    temp_sense_pre_time = curr_time; 
  }
}

// LCD display
void display(){
    loader_icon_counter++;

    lcd.clear();
    
    //line 1
    lcd.setCursor(0, 0);
    if(selected_phase == 1)
        lcd.print("Yellowing 4.00H");
    else if(selected_phase == 2)
        lcd.print("Color fixing 10.00H");
    else if(selected_phase == 3)
        lcd.print("Lamina 12.30H");
    else if(selected_phase == 4)
        lcd.print("Stem 10.00H");
    else{
        lcd.print("No phase selected");
        return;
    }
    
    // Line 2
    lcd.setCursor(0, 1);
    lcd.print("Dry:");
    lcd.print(phase[selected_phase-1][hour][0]);     //middle hour
    lcd.print("-->");
    lcd.print(tempF[0]);
    lcd.print("F");

    // Line 3
    lcd.setCursor(0, 2);
    lcd.print("Wet:");
    lcd.print(phase[selected_phase-1][hour][1]);     //middle hour
    lcd.print("-->");
    lcd.print(tempF[1]);
    lcd.print("F");

    // Line 4
    lcd.setCursor(0, 3);
    lcd.print("Time: ");
    lcd.print(hour);     // here will be hour
    lcd.print(":");
    lcd.print(min);     // here will be min
}


// For reset millis
void setMillis(unsigned long new_millis){
  uint8_t oldSREG = SREG;
  cli();
  timer0_millis = new_millis;
  SREG = oldSREG;
}