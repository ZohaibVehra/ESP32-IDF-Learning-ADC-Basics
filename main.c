/* ADC1 Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static esp_adc_cal_characteristics_t *adc_chars;    //Structure storing characteristics of an ADC.
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2 ./ ADC ch 6 is by defualt gpio pin 34, this isnt changeable 
static const adc_atten_t atten = ADC_ATTEN_DB_6;    //parameter that determines the range of the adc. 0 as it is now is for up to 800mV i think
static const adc_unit_t unit = ADC_UNIT_1;  //adc 1 or 2 

/*
To recap, so far we have we have created a pointer called adc_chars, which will store the resto f the info
Set our adc channel to 6, which is connected to gpio 34
set the range to the minimum at 0-800mV
and finally set our adc to 1
*/


static void check_efuse()
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {  //ESP_ADC_CAL_VAL_EFUSE_TP is an enum from the #include "esp_adc_cal.h" on line 14
        //esp_adc_cal_check_efuse checks if adc callibration vals are burned into efuse. if ret is ESP_OK then good, ESP_ERR_NOT_SUPPORTED means no, and ESP_ERR_INVALID_ARG is error
        
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}


static void print_char_val_type(esp_adc_cal_value_t val_type)   //esp_adc_cal_value_t refrs to type of calibration val, can calibrate from given efuse val, from defeault ref V, etc.
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
    //self explanatory, ESP_ADC_CAL_VAL_EFUSE_TP means calibrate using 2 point value stored in eFuse, wheres the next else if statement is for default eFuse Vref/
}

void app_main()
{
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();  //lets us know if 2 port or Vref are burned in

    //Configure ADC
    if (unit == ADC_UNIT_1) {   // it is we defined this at start to mean use adc 1
        adc1_config_width(ADC_WIDTH_BIT_12); //Configure ADC1 capture width, meanwhile enable output invert for ADC1. The configuration is for all channels of ADC1. we set to 12 bits
        adc1_config_channel_atten(channel, atten); //just setting up from what we wrote at the start
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten); //if not using adc 1 this is to set up adc2. notice we change channel to type adc2_channel_t
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t)); 
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars); //hover its simple enough
    print_char_val_type(val_type);

    //Continuously sample ADC1
    while (1) {
        uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            if (unit == ADC_UNIT_1) {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);   //the adc1_get_raw func can also return -1 for parameter error. unit for reading is unit32_t btw
            } else {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
                adc_reading += raw;
            }
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);  
        printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}