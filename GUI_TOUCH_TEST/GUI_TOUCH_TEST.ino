// Project: BAT project
// Code Author: Dhrubo Roy Partho
// Date: 18/06/2024
// Version: 2.0v
// Upgrade: touch display + log data bug fix

#include <FS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "lol.h"
#include <OneWire.h>
#include <DallasTemperature.h>

TFT_eSPI tft = TFT_eSPI();

#define CALIBRATION_FILE "/TouchCalData1"

#define REPEAT_CAL false

// Specially for Home Interface
#define offset_row 35
#define offset_col 5
#define LINE tft.setCursor(offset_col, offset_row + tft.getCursorY())
#define space (char)32

// Keypad start position, home_key sizes and spacing
#define NORMAL_KEY_X 117
#define NORMAL_KEY_Y 243
#define NORMAL_KEY_W 215
#define NORMAL_KEY_H 35
#define NORMAL_KEY_SPACING_X 18
#define NORMAL_KEY_SPACING_Y 10
#define KEY_TEXTSIZE 1

// manual keypad
#define KEY_X 40 // Centre of key
#define KEY_Y 115  // 96
#define KEY_W 62 // Width and height
#define KEY_H 35
#define KEY_SPACING_X 18 // X and Y gap
#define KEY_SPACING_Y 10

// Numeric display box size and location
#define DISP_X 1
#define DISP_Y 10
#define DISP_W 238
#define DISP_H 50
#define DISP_TSIZE 3
#define DISP_TCOLOR TFT_CYAN

#define MANUAL_BTN_PADDING_Y 75

#define DATA_PRINT_W    115
#define DATA_PRINT_H    30

char home_key_label[3][10] = {
    {173, 106, 101, 164, 0},    // মেনু
    {252, 117, 119, 162, 72, 178, 117, 0},   //স্বয়ংক্রিয়
    {173, 106, 101, 164, 117, 161, 109, 0}   //মেনুয়াল  
};


char manual_key_label[5][11] = {
    {110, 164, 76, 101, 161, 32, 97, 161, 102, 0},    // শুকনো তাপ
    {174, 105, 83, 161, 32, 97, 161, 102, 0},   // ভেজা তাপ
    {112, 106, 117, 0},     // সময়
    {174, 113, 161, 106, 0},   // হোম  
    {74, 173, 76, 0}   // ওকে  
};


char phase_label[5][15] = {
    {173, 101, 67, 0},      // নেই
    {67, 173, 117, 161, 109, 162, 117, 119, 0},     // ইয়োলয়িং
    {76, 161, 109, 161, 108, 32, 162, 103, 76, 124},    // কালার ফিক.
    {173, 109, 162, 106, 101, 161, 0},              // লেমিনা
    {173, 248, 106, 0}      // স্টেম
};


// Phase temperature
const byte phase_temp[4][14][2] = {{{95, 92}, {96, 93}, {98, 94}, {99, 95}, {100, 96}},
                              {{100, 96}, {102, 96}, {104, 97}, {106, 97}, {108, 98}, {110, 98}, {112, 98}, {114, 99}, {116, 99}, {118, 100}, {120, 100}},
                              {{120, 100}, {122, 100}, {124, 101}, {126, 101}, {128, 102}, {130, 102}, {132, 102}, {134, 103}, {136, 103}, {138, 104}, {140, 104}, {142, 104}, {144, 105}, {145, 105}},
                              {{145, 105}, {147, 106}, {151, 107}, {153, 107}, {155, 108}, {157, 108}, {159, 109}, {161, 109}, {163, 110}, {163, 110}, {165, 110}}};

const byte phase_duration_hour[4] = {0, 0, 0, 0};    //4 10 12 10
const byte phase_duration_min[4] = {0, 0, 0, 0};   //0 0 30 0
const byte phase_duration_sec[4] = {10, 10, 10, 10};

// For key pad
#define NUM_LEN 12
char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

// We have a status line for messages
#define STATUS_X 120 // Centred on this
#define STATUS_Y 65

// Create 15 keys for the keypad
char keyLabel[15][5] = {{173, 106, 101, 164, 0}, 
                        {162, 88, 109, 124, 0}, 
                        {74, 174, 76, 0}, 
                        "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", ":" };
uint16_t keyColor[15] = {TFT_RED, TFT_RED, TFT_RED,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE
                        };

// Invoke the TFT_eSPI button class and create all the button objects
TFT_eSPI_Button key[15];
// keypad end

TFT_eSPI_Button home_key[2];
TFT_eSPI_Button manual_key[5];


// Some global variable
uint64_t cur_time = 0;
uint64_t pre_time = 0;
uint8_t sec = 0;
uint8_t minute = 0;
uint8_t hour = 0;

uint8_t limit_sec = 0;
uint8_t limit_min = 0;
uint8_t limit_hour = 0;

uint8_t final_limit_sec = 0;
uint8_t final_limit_min = 0;
uint8_t final_limit_hour = 0;

uint64_t limit_time_ms = 0;

bool cur_clock_flag = false;

uint8_t final_dry_temp = 0;
uint8_t final_wet_temp = 0;

byte selected_mode = 0;
byte selected_phase = 0;
byte selected_pre_phase = -1;
byte selected_interface = 1;    // 1 for Home
int pre_interface = 0;
byte manual_changing_option = 0;

// char dry_change_temp[6];
// char wet_change_temp[6];
// char changed_time[8];

int16_t changed_dry_temp = 0;
int16_t changed_wet_temp = 0;
uint64_t changed_time_limit = 0;


// temperature objects and variables
#define temp1 16
#define temp2 17

// Onewire objects 
OneWire oneWire1(temp1);
OneWire oneWire2(temp2);

// DallasTemperature objects
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);

float tempF[2] = {0, 0};


// process stop or not
bool work_process = true;


void setup() {
    Serial.begin(115200);

    tft.init();

    tft.setRotation(0);

    touch_calibrate();

    tft.setFreeFont(&lol);

    tft.fillScreen(TFT_BLACK);
    // tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);

    // home_interface();
    // homeKeypad();

    // manual_interface();

    // draw_display();
    // drawKeypad();
}

void loop() {
    // home_touch();
    phase_control();
    cur_time = millis();
    if(cur_time - pre_time >= 970 && selected_interface == 1 && cur_clock_flag){
        pre_time = cur_time;
        sec++;
        if(sec == 60){
            minute++;
            sec = 0;
            if(minute == 60){
                hour++;
                minute = 0;
            }
        }
        cur_time_print();
    }
    // manual_keypad_touch();
    interface_control();
    touch_control();    
    temp_measure();
    // temp_control();
}

// // Time controller function
// void time_controller() {
//     if(limit_time_ms <= cur_time){
//         // phase change or manual phase off
//         // 10 sec warning
//         int ender = 3;
//         while(ender--){
//             delay(2000);
//             // warning;
//         }
//         // byte selected_mode = 1;
//         // byte selected_phase = 0;
//         if(selected_mode == 1){
//             selected_phase++;
//         }
//     }
// }

// Temperature measurement function
void temp_measure() {
    static uint64_t temp_pre_time = 0; 
    if(cur_time - temp_pre_time >= 2000 && selected_interface == 1){
        sensor1.requestTemperatures();
        // sensor2.requestTemperatures();

        tempF[0] = sensor1.getTempFByIndex(0);
        // tempF[1] = sensor2.getTempFByIndex(0);

        Serial.print(sensor1.getTempFByIndex(0));
        // Serial.print(" ");
        // Serial.println(sensor2.getTempFByIndex(0));

        dry_temp_print();
        wet_temp_print();

        temp_pre_time = cur_time;
    }
}

uint64_t time_to_ms(int h, int m, int s) {
    return static_cast<uint64_t>((h*3600000) + (m*60000) + (s*1000));
}

void phase_control() {
    // Serial.println(limit_time_ms);
    if(limit_time_ms < time_to_ms(hour, minute, sec) && selected_mode == 2){
        warning_section(2);
        hour = 0;
        minute = 0;
        sec = 0;
        final_limit_hour = 0;
        final_limit_min = 0;
        final_limit_sec = 0;
        cur_clock_flag = false;
        selected_phase = 0;
        selected_pre_phase = 0;
        selected_mode = 0;
        time_limit_print();
        phase_print();
        cur_time_print();
        mode_print(0);
    }

    else if(limit_time_ms < time_to_ms(hour, minute, sec) && selected_mode == 1){
        selected_phase++;
        if(selected_phase >= 5){
            warning_section(2);
            hour = 0;
            minute = 0;
            sec = 0;
            final_limit_hour = 0;
            final_limit_min = 0;
            final_limit_sec = 0;
            selected_phase = 0;
            selected_pre_phase = 0;
            selected_mode = 0;
            cur_clock_flag = false;
            time_limit_print();
            phase_print();
            cur_time_print();
            mode_print(0);
        } else{
            warning_section(1);
            // Setting time according to phase
            switch(selected_phase){
                case 1:
                    hour = 0;
                    minute = 0;
                    sec = 0;
                    final_limit_hour = phase_duration_hour[0];
                    final_limit_min = phase_duration_min[0];
                    final_limit_sec = phase_duration_sec[0];
                    limit_time_ms = time_to_ms(final_limit_hour, final_limit_min, final_limit_sec);
                    pre_time = millis();
                    cur_clock_flag = true;
                    cur_time_print();
                    Serial.println("hello");
                    Serial.println(limit_time_ms);
                    break;
                case 2:
                    hour = 0;
                    minute = 0;
                    sec = 0;
                    final_limit_hour = phase_duration_hour[1];
                    final_limit_min = phase_duration_min[1];
                    final_limit_sec = phase_duration_sec[1];
                    limit_time_ms = time_to_ms(final_limit_hour, final_limit_min, final_limit_sec);
                    pre_time = millis();
                    cur_clock_flag = true;
                    cur_time_print();
                    Serial.println(limit_time_ms);
                    break;
                case 3:
                    hour = 0;
                    minute = 0;
                    sec = 0;
                    final_limit_hour = phase_duration_hour[2];
                    final_limit_min = phase_duration_min[2];
                    final_limit_sec = phase_duration_sec[2];
                    limit_time_ms = time_to_ms(final_limit_hour, final_limit_min, final_limit_sec);
                    pre_time = millis();
                    cur_clock_flag = true;
                    cur_time_print();
                    Serial.println(limit_time_ms);
                    break;
                case 4:
                    hour = 0;
                    minute = 0;
                    sec = 0;
                    final_limit_hour = phase_duration_hour[3];
                    final_limit_min = phase_duration_min[3];
                    final_limit_sec = phase_duration_sec[3];
                    limit_time_ms = time_to_ms(final_limit_hour, final_limit_min, final_limit_sec);
                    pre_time = millis();
                    cur_clock_flag = true;
                    cur_time_print();
                    Serial.println(limit_time_ms);
                    break;
            }
            time_limit_print();
            phase_print();
        }
    }
}

void warning_section(int warning_flag) {
    //warning_flag = 1; Phase changing
    //warning_flag = 2; Time exit/fullfilled
    (warning_flag == 1) ? warning_flag = 4 : warning_flag = 8;
    for(int i=0;i<warning_flag;i++){
        tft.fillScreen(TFT_GREEN);
        delay(700);
        tft.fillScreen(TFT_BLACK);
        delay(700);
    }
    tft.fillScreen(TFT_BLACK);
    pre_interface = -1;
    interface_control();
    touch_control();  
}

void temp_control() {

}

void touch_control() {
    switch(selected_interface){
        case 1:
            home_touch();
            break;
        case 2:
            manual_touch();
            break;
        case 3:
            manual_keypad_touch();
            break;
    }
}

void interface_control() {
    if(pre_interface == selected_interface) return;
    switch(selected_interface){
        case 1:
            home_interface();
            homeKeypad();
            break;
        case 2:
            manual_interface();
            break;
        case 3:
            draw_display();
            drawKeypad();
            break;
    }
    pre_interface = selected_interface;
}

void draw_display() {
    tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);

    tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_BLACK);
    tft.drawRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);
}

void manual_keypad_touch() {
    uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = tft.getTouch(&t_x, &t_y);

  // / Check if any key coordinate boxes contain the touch coordinates
  for (uint8_t b = 0; b < 15; b++) {
    if (pressed && key[b].contains(t_x, t_y)) {
      key[b].press(true);  // tell the button it is pressed
    } else {
      key[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 15; b++) {

    // if (b < 3) tft.setFreeFont(LABEL1_FONT);
    // else tft.setFreeFont(LABEL2_FONT);
    tft.setFreeFont(&lol);

    if (key[b].justReleased()) key[b].drawButton();     // draw normal

    if (key[b].justPressed()) {
      key[b].drawButton(true);  // draw invert

      // if a numberpad button, append the relevant # to the numberBuffer
      if (b >= 3) {
        if (numberIndex < NUM_LEN) {
          numberBuffer[numberIndex] = keyLabel[b][0];
          numberIndex++;
          numberBuffer[numberIndex] = 0; // zero terminate
        }
        status(""); // Clear the old status
      }

      // Del button, so delete last char
      if (b == 1) {
        numberBuffer[numberIndex] = 0;
        if (numberIndex > 0) {
          numberIndex--;
          numberBuffer[numberIndex] = 0;//' ';
        }
        status(""); // Clear the old status
      }

      if (b == 2) {
        // status("Sent value to serial port");
        Serial.println(numberBuffer);

        switch(manual_changing_option){
            case 0:
                break;
            case 1:
                changed_dry_temp = convert_str_to_int(numberBuffer);
                // numberBuffer[0] = 0;
                break;
            case 2:
                changed_wet_temp = convert_str_to_int(numberBuffer);
                // numberBuffer[0] = 0;
                break;
            case 3:
                if(convert_str_to_uint(numberBuffer) == 1) break;
                changed_time_limit = convert_str_to_uint(numberBuffer);
                // numberBuffer[0] = 0;
            default:
                break;
        }

        numberIndex = 0;
        numberBuffer[numberIndex] = 0;

        selected_interface = 2;

      }
      // we dont really check that the text field makes sense
      // just try to call
      if (b == 0) {
        selected_interface = 2;     // Manual menu interface

        // // status("Value cleared");
        // numberIndex = 0; // Reset index to 0
        // numberBuffer[numberIndex] = 0; // Place null in buffer
      }

      // Update the number display field
      tft.setTextDatum(TL_DATUM);        // Use top left corner as text coord datum
    //   tft.setFreeFont(&FreeSans18pt7b);  // Choose a nice font that fits box
        tft.setFreeFont(&lol);
      tft.setTextColor(DISP_TCOLOR);     // Set the font colour

      // Draw the string, the value returned is the width in pixels
      int xwidth = tft.drawString(numberBuffer, DISP_X + 4, DISP_Y + 12);

      // Now cover up the rest of the line up by drawing a black rectangle.  No flicker this way
      // but it will not work with italic or oblique fonts due to character overlap.
      tft.fillRect(DISP_X + 4 + xwidth, DISP_Y + 1, DISP_W - xwidth - 5, DISP_H - 2, TFT_BLACK);

      delay(10); // UI debouncing
    }
  }
}

int16_t convert_str_to_int(char bffr[]) {
    int c_temp = 0;
    int factor = 1;
    int cnt = 0;
    for(cnt=0;bffr[cnt];cnt++){
        if(bffr[cnt] == ':' || bffr[cnt] == '.'){
            break;
        }
    }
    for(int i=cnt-1;i>=0;i--){
        c_temp += factor * (int)(bffr[i] - '0');
        factor *= 10;
    }
    return c_temp;
}

uint64_t convert_str_to_uint(char buffer[]) {
    String temp_time[3];
    int j = 0;
    int colon_cnt = 0;
    for(int i=0;buffer[i];i++){
        if(buffer[i] == ':' || buffer[i] == '.'){
            j++; colon_cnt++;
        }else if(buffer[i] >= '0' && buffer[i] <= '9')
            temp_time[j] += buffer[i];
    }

    if(colon_cnt != 2) return static_cast<uint64_t>(1);

    int temp_hour = 0;
    int temp_min = 0;
    int temp_sec = 0;
    int factor = 1;
    for(int i = temp_time[0].length() - 1; i>=0;i--){
        Serial.print("temp_time[0]: ");
        Serial.println((int)(temp_time[0][i] - 48));
        temp_hour += factor * (int)(temp_time[0][i] - 48);
        factor *= 10;
    }
    // Serial.print("Process hour: (temp_hour) ");
    // Serial.println(temp_hour);
    factor = 1;
    for(int i = temp_time[1].length() - 1; i>=0;i--){
        Serial.print("temp_time[1]: ");
        Serial.println((int)(temp_time[1][i] - 48));
        temp_min += factor * (int)(temp_time[1][i] - 48);
        factor *= 10;
    }
    // Serial.print("Process min: (temp_min) ");
    // Serial.println(temp_min);
    factor = 1;
    for(int i = temp_time[2].length() - 1; i>=0;i--){
        Serial.print("temp_time[2]: ");
        Serial.println((int)(temp_time[2][i] - 48));
        temp_sec += factor * (int)(temp_time[2][i] - 48);
        factor *= 10;
    }

    if(temp_min >= 60) return 1;
    if(temp_sec >= 60) return 1;

    // Serial.print("Process sec: (temp_sec) ");
    // Serial.println(temp_sec);
    limit_hour = temp_hour;
    limit_min = temp_min;
    limit_sec = temp_sec;
    // Serial.println();
    // Serial.print(temp_time[0]);
    // Serial.print(temp_time[1]);
    // Serial.println(temp_time[2]);
    // Serial.print(temp_hour);
    // Serial.print(temp_min);
    // Serial.println(temp_sec);
    Serial.println(((temp_hour * 3600000) + (60000 * temp_min) + (temp_sec * 1000)) * 1LL);
    return static_cast<uint64_t>((temp_hour * 3600000) + (60000 * temp_min) + (temp_sec * 1000));
}

// Print something in the mini status bar
void status(const char *msg) {
    tft.setTextPadding(240);
    //tft.setCursor(STATUS_X, STATUS_Y);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    //   tft.setTextFont(0);
    tft.setFreeFont(&lol);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(1);
    tft.drawString(msg, STATUS_X, STATUS_Y);
    // drawKeypad();
}

// Manual keypad
void drawKeypad() {
    selected_interface = 3;
  // Draw the keys
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t b = col + row * 3;

    //   if (b < 3) tft.setFreeFont(LABEL1_FONT);
    //   else tft.setFreeFont(LABEL2_FONT);
    tft.setFreeFont(&lol);

      key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
                        KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
                        KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
                        keyLabel[b], KEY_TEXTSIZE);
      key[b].drawButton();
    }
  }
}

void manual_dry_temp() {
    tft.fillRect(98, 0, 150, 25, TFT_RED);
    tft.print(changed_dry_temp);
}

void manual_wet_temp() {
    tft.fillRect(84, 30, 155, 25, TFT_RED);
    tft.print(changed_wet_temp);
}

void manual_limit_time() {
    tft.fillRect(74, 60, 155, 25, TFT_RED);
    // tft.print(changed_time_limit);

    if(limit_hour < 10) tft.print(0);
    tft.print(limit_hour);
    
    tft.print(":");
    
    if(limit_min < 10) tft.print(0);
    tft.print(limit_min);

    tft.print(":");

    if(limit_sec < 10) tft.print(0);
    tft.print(limit_sec);
}

void manual_interface() {
    selected_interface = 2;
    tft.fillScreen(TFT_BLACK);

    tft.setCursor(offset_col, 20);
    dry_print();
    manual_dry_temp();

    tft.setCursor(offset_col, 30 + tft.getCursorY());
    wet_print();
    manual_wet_temp();

    tft.setCursor(offset_col, 30 + tft.getCursorY());
    time_label_print();
    manual_limit_time();
    
    manual_key[0].initButton(&tft, 120, NORMAL_KEY_H + MANUAL_BTN_PADDING_Y, 240, NORMAL_KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE, manual_key_label[0], KEY_TEXTSIZE);
    manual_key[0].drawButton();

    manual_key[1].initButton(&tft, 120, 2 * NORMAL_KEY_H + NORMAL_KEY_SPACING_Y + MANUAL_BTN_PADDING_Y, 240, NORMAL_KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE, manual_key_label[1], KEY_TEXTSIZE);
    manual_key[1].drawButton();

    manual_key[2].initButton(&tft, 120, 3 * NORMAL_KEY_H + 2 * NORMAL_KEY_SPACING_Y + MANUAL_BTN_PADDING_Y, 240, NORMAL_KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE, manual_key_label[2], KEY_TEXTSIZE);
    manual_key[2].drawButton();

    manual_key[3].initButton(&tft, 120, 4 * NORMAL_KEY_H + 3 * NORMAL_KEY_SPACING_Y + MANUAL_BTN_PADDING_Y, 240, NORMAL_KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE, manual_key_label[3], KEY_TEXTSIZE);
    manual_key[3].drawButton();

    manual_key[4].initButton(&tft, 120, 5 * NORMAL_KEY_H + 4 * NORMAL_KEY_SPACING_Y + MANUAL_BTN_PADDING_Y, 240, NORMAL_KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE, manual_key_label[4], KEY_TEXTSIZE);
    manual_key[4].drawButton();
}

void manual_touch() {
    uint16_t t_x = 0, t_y = 0;

    bool pressed = tft.getTouch(&t_x, &t_y);

    if(pressed && manual_key[0].contains(t_x, t_y)){
        Serial.println("manual_key 1 pressed");
        manual_key[0].press(true);
    }else{
        manual_key[0].press(false);
    }

    if(pressed && manual_key[1].contains(t_x, t_y)){
        Serial.println("manual_key 2 pressed");
        manual_key[1].press(true);
    }else{
        manual_key[1].press(false);
    }

    if(pressed && manual_key[2].contains(t_x, t_y)){
        Serial.println("manual_key 3 pressed");
        manual_key[2].press(true);
    }else{
        manual_key[2].press(false);
    }

    if(pressed && manual_key[3].contains(t_x, t_y)){
        Serial.println("manual_key 4 pressed");
        manual_key[3].press(true);
    }else{
        manual_key[3].press(false);
    }

    if(pressed && manual_key[4].contains(t_x, t_y)){
        Serial.println("manual_key 5 pressed");
        manual_key[4].press(true);
    }else{
        manual_key[4].press(false);
    }


    if(manual_key[0].justReleased()) manual_key[0].drawButton();
    if(manual_key[0].justPressed()) manual_key[0].drawButton(true);
    if(manual_key[1].justReleased()) manual_key[1].drawButton();
    if(manual_key[1].justPressed()) manual_key[1].drawButton(true);
    if(manual_key[2].justReleased()) manual_key[2].drawButton();
    if(manual_key[2].justPressed()) manual_key[2].drawButton(true);
    if(manual_key[3].justReleased()) manual_key[3].drawButton();
    if(manual_key[3].justPressed()) manual_key[3].drawButton(true);
    if(manual_key[4].justReleased()) manual_key[4].drawButton();
    if(manual_key[4].justPressed()) manual_key[4].drawButton(true);
    delay(10);

    if(manual_key[0].justReleased()) {
        selected_interface = 3;
        manual_changing_option = 1;
    }
    if(manual_key[1].justReleased()) {
        selected_interface = 3;
        manual_changing_option = 2;
    }
    if(manual_key[2].justReleased()) {
        selected_interface = 3;
        manual_changing_option = 3;
    }
    if(manual_key[3].justReleased()) {
        selected_interface = 1;
        manual_changing_option = 0;
    }

    // Ok Button
    if(manual_key[4].justReleased()) {
        if(changed_dry_temp < changed_wet_temp && (limit_hour != 0 || limit_min != 0 || limit_sec > 10)){
            // interface, phase, option, mode
            selected_interface = 1;
            manual_changing_option = 0;
            selected_mode = 2;
            selected_phase = 0;

            // Changing the main time limit
            final_limit_hour = limit_hour;
            final_limit_min = limit_min;
            final_limit_sec = limit_sec;

            //setting time to ms for comparison
            limit_time_ms = time_to_ms(final_limit_hour, final_limit_min, final_limit_sec);

            // Changing the main temp controller
            final_dry_temp = changed_dry_temp;
            final_wet_temp = changed_wet_temp;
            Serial.print("dry temp: ");
            Serial.println(final_dry_temp);
            Serial.print("wet temp: ");
            Serial.println(final_wet_temp);

            // Reseting the time of current section in Home interface
            hour = 0;
            minute = 0;
            sec = 0;
            pre_time = millis();

            // clock timer start
            cur_clock_flag = true;
        }
        else{
            Serial.println("Requirement not fullfilled");
        }
    }
}

void home_interface() {
    selected_interface = 1;
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 240, 320, TFT_BLACK);
    tft.setCursor(offset_col, offset_row - 10);
    dry_print();
    dry_temp_print();
    // drawBitmap(tft.getCursorX(), tft.getCursorY(), degreeBitmap, 14, 14);
    farenheit_print(215, 5);
    LINE;
    wet_print();
    wet_temp_print();
    farenheit_print(215, 5 + 35);
    LINE;
    time_label_print();
    cur_time_print();
    LINE;
    timeend_label_print();
    time_limit_print();
    LINE;
    phase_label_print();
    phase_print();
    LINE;
    mode_label_print();
    mode_print(selected_mode);
}

void farenheit_print(int x, int y) {
    tft.setTextFont(2);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("°F", x, y);
    tft.setTextSize(1);
    tft.setFreeFont(&lol);
}

void cur_time_print() {
    tft.fillRect(68, offset_col + 2*DATA_PRINT_H + 10, 150, DATA_PRINT_H, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(67 + 5, 95);
    if(hour < 10){
        tft.print('0');
    }
    tft.print(hour);
    tft.print(":");
    if(minute < 10){
        tft.print('0');
    }
    tft.print(minute);
    tft.print(":");
    if(sec < 10){
        tft.print('0');
    }
    tft.print(sec);
}

void time_limit_print() {
    // tft.fillRect(67, 95 - 20, DATA_PRINT_W, DATA_PRINT_H, TFT_RED);
    tft.fillRect(68, offset_col + 3*DATA_PRINT_H + 15, 150, DATA_PRINT_H, TFT_BLACK);
    // tft.setCursor(offset_col + 70, offset_row - 10);
    // tft.setCursor(c_x + 5, c_y);
    tft.setCursor(67 + 5, 130);
    // Serial.println((int)tft.getCursorX());
    // Serial.println((int)tft.getCursorY());
    // tft.print("12:59:43");
    if(final_limit_hour < 10) tft.print(0);
    tft.print(final_limit_hour);
    
    tft.print(":");
    
    if(final_limit_min < 10) tft.print(0);
    tft.print(final_limit_min);

    tft.print(":");

    if(final_limit_sec < 10) tft.print(0);
    tft.print(final_limit_sec);
}

// 1 for automatic. 2 for manual
void mode_print(int mode_selection) {
    tft.fillRect(72, offset_col + 5*DATA_PRINT_H + 25, 150, DATA_PRINT_H, TFT_BLACK);
    tft.setCursor(70 + 5, 200);
    // Serial.println((int)tft.getCursorX());
    // Serial.println((int)tft.getCursorY());
    switch(mode_selection){
        case 1:
            auto_mode_print();
            break;
        case 2:
            manual_mode_print();
            break;
    }
}

void phase_print() {
    // tft.fillRect(67, 95 - 20, DATA_PRINT_W, DATA_PRINT_H, TFT_RED);
    tft.fillRect(72, offset_col + 4*DATA_PRINT_H + 20, 150, DATA_PRINT_H, TFT_BLACK);
    // tft.setCursor(offset_col + 70, offset_row - 10);
    // tft.setCursor(c_x + 5, c_y);
    tft.setCursor(72 + 5, 165);
    // Serial.println((int)tft.getCursorX());
    // Serial.println((int)tft.getCursorY());
    // tft.print("12:59:43");
    tft.print(phase_label[selected_phase]);
}

void dry_temp_print() {
    tft.fillRect(offset_row + 60, offset_col, DATA_PRINT_W, DATA_PRINT_H, TFT_BLACK);
    // tft.setCursor(offset_col + 70, offset_row - 10);
    // tft.setCursor(c_x + 5, c_y);
    // tft.setCursor(tft.getCursorX(), tft.getCursorY());
    tft.setCursor(98, 25);
    // Serial.println((int)tft.getCursorX());
    // Serial.println((int)tft.getCursorY());
    // tft.print("-196.00");
    tft.print(tempF[0]);
}

void wet_temp_print() {
    tft.fillRect(offset_row + 60, offset_col + DATA_PRINT_H + 5, DATA_PRINT_W, DATA_PRINT_H, TFT_BLACK);
    // tft.setCursor(offset_col + 70, offset_row - 10);
    // tft.setCursor(c_x + 5, c_y);
    // tft.setCursor(tft.getCursorX(), tft.getCursorY());
    tft.setCursor(98, 60);   //77->x
    // Serial.println((int)tft.getCursorX());
    // Serial.println("#");
    // Serial.println((int)tft.getCursorY());
    // tft.print("100.9");
    tft.print(tempF[1]);

    // tft.setTextFont(2);
    // tft.setTextSize(2);
    // tft.print("F");
    // tft.setTextSize(1);
    // tft.setFreeFont(&lol);
}

void home_touch(){
    uint16_t t_x = 0, t_y = 0;

    bool pressed = tft.getTouch(&t_x, &t_y);

    if(pressed && home_key[0].contains(t_x, t_y)){
        Serial.println("home_key 1 pressed");
        home_key[0].press(true);
    }
    else{
        home_key[0].press(false);
    }
    if(pressed && home_key[1].contains(t_x, t_y)){
        Serial.println("home_key 2 pressed");
        home_key[1].press(true);
    }else{
        home_key[1].press(false);
    }

    if(home_key[0].justReleased()) home_key[0].drawButton();
    if(home_key[0].justPressed()) home_key[0].drawButton(true);
    if(home_key[1].justReleased()) home_key[1].drawButton();
    if(home_key[1].justPressed()) home_key[1].drawButton(true);
    delay(10);

    // Auto mode
    if(home_key[0].justReleased()) {
        // here all will be reset
        selected_interface = 1;
        selected_mode = 1;
        selected_phase = 1;

        // Current time reseting
        hour = 0;
        minute = 0;
        sec = 0;

        pre_time = millis();


        // Setting time
        final_limit_hour = phase_duration_hour[selected_phase - 1];
        final_limit_min = phase_duration_min[selected_phase - 1];
        final_limit_sec = phase_duration_sec[selected_phase - 1];

        //setting time to ms for comparison
        limit_time_ms = time_to_ms(final_limit_hour, final_limit_min, final_limit_sec);

        // cur time flag on
        cur_clock_flag = true;


        home_interface();
        homeKeypad();
    }
    // Manual mode
    if(home_key[1].justReleased()) {
        selected_interface = 2;
        limit_hour = 0;
        limit_min = 0;
        limit_sec = 0;

        changed_dry_temp = 0;
        changed_wet_temp = 0;
    }
}

void homeKeypad() {
    home_key[0].initButton(&tft, NORMAL_KEY_X, NORMAL_KEY_Y, NORMAL_KEY_W, NORMAL_KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE, home_key_label[1], KEY_TEXTSIZE);
    home_key[0].drawButton();

    home_key[1].initButton(&tft, NORMAL_KEY_X, NORMAL_KEY_Y + NORMAL_KEY_H + NORMAL_KEY_SPACING_Y, NORMAL_KEY_W, NORMAL_KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE, home_key_label[2], KEY_TEXTSIZE);
    home_key[1].drawButton();

    // for(uint8_t row = 0; row < 1; row++){
    //     for(uint8_t col = 0; col < 2; col++){
    //         uint8_t b = col + row*3;

    //         home_key[b].initButton(&tft, NORMAL_KEY_X + col * (NORMAL_KEY_W + NORMAL_KEY_SPACING_X),
    //                 NORMAL_KEY_Y + row * (NORMAL_KEY_H + NORMAL_KEY_SPACING_Y), 
    //                 NORMAL_KEY_W, NORMAL_KEY_H, TFT_WHITE, TFT_RED, TFT_WHITE, home_key_label[b], KEY_TEXTSIZE);
    //         home_key[b].drawButton();
    //     }
    // }
}

void touch_calibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

// dry write print
void dry_print(){
    tft.write(110);
    tft.write(164);
    tft.write(76);
    tft.write(173);
    tft.write(101);
    tft.write(161);
    tft.write(120);
    tft.write(space);
    return;
}

void wet_print(){
    tft.write(173);
    tft.write(105);
    tft.write(83);
    tft.write(161);
    tft.write(120);
    tft.write(space);
    return;
}

void far_print(){
    tft.print((char)103);
    tft.print((char)161);
    tft.print((char)46);
    return;
}

void mode_label_print() {
    tft.write(173);
    tft.write(106);
    tft.write(161);
    tft.write(88);
    tft.write(120);
}

void manual_mode_print(){
    tft.print(space);

    tft.print((char)173);
    tft.print((char)106);
    // tft.print((char)201);
    // tft.print((char)161);
    tft.print((char)101);
    tft.print((char)164);
    tft.print((char)117);
    tft.print((char)161);
    tft.print((char)109);
    return;
}

void auto_mode_print(){
    tft.print(space);

    tft.print((char)252);
    tft.print((char)117);
    tft.print((char)119);
    tft.print((char)162);
    tft.print((char)72);
    tft.print((char)178);
    tft.print((char)117);
    
    return;
}

void time_label_print() {
    tft.write(112);
    tft.write(106);
    tft.write(117);
    tft.write(120);
    tft.write(space);
    return;
}

void timeend_label_print() {
    // tft.write(112);
    // tft.write(106);
    // tft.write(117);
    tft.write(112);
    tft.write(163);
    tft.write(106);
    tft.write(161);
    tft.write(120);
    tft.write(space);
    return;
}

void phase_label_print() {
    tft.write(173);
    tft.write(103);
    tft.write(83);
    tft.write(120);
    return;
}

// Bitmap draw function
void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
    uint8_t byte = 0;
    int16_t byteIndex = 0;
    
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i % 8 == 0) {
                byte = pgm_read_byte(&bitmap[byteIndex++]);
            }
            if (byte & 0x80) {
                tft.drawPixel(x + i, y + j, color);
            }
            byte <<= 1;
        }
    }
}