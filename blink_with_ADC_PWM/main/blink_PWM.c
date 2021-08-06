/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "driver/timer.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "Log_data.h"

#define GPIO_OUTPUT_LED		27
#define GPIO_OUTPUT_SPARE   17
#define PUSHBUTTON			19

void IRAM_ATTR gpio_isr_handler(void* arg);
void IRAM_ATTR timer_isr_handler(void* arg);

static intr_handle_t s_timer_handle;

unsigned long int Actual_time = 0 ;
unsigned int Actual_time_int32 = 0, i=0  ;
uint32_t Timer_Low_value = 0, Timer_High_value = 0, Elapsed_time = 0, Actual_Timer_time = 0, GPIO_Status_main = 0, GPIO_Status_int = 0  ;
uint32_t ADC_CH0_value = 0, Int_counter = 0 ;


void app_main(void)
{
    gpio_config_t GPIO_Config;
    GPIO_Config.intr_type = GPIO_INTR_DISABLE;
    GPIO_Config.mode = GPIO_MODE_OUTPUT;
    GPIO_Config.pin_bit_mask = (1ULL<<GPIO_OUTPUT_LED) ;//| (1ULL<<GPIO_OUTPUT_SPARE);
    GPIO_Config.pull_down_en = 0;
    GPIO_Config.pull_up_en = 0;
    gpio_config(&GPIO_Config);

    GPIO_Config.intr_type = GPIO_INTR_NEGEDGE;
    GPIO_Config.mode = GPIO_MODE_INPUT;
    GPIO_Config.pin_bit_mask = (1ULL<<PUSHBUTTON) ;
    GPIO_Config.pull_down_en = 0;
    GPIO_Config.pull_up_en = 0;
    gpio_config(&GPIO_Config);

    timer_config_t Timer_Config;
    Timer_Config.alarm_en = TIMER_ALARM_EN;
    Timer_Config.auto_reload = TIMER_AUTORELOAD_EN;
    Timer_Config.divider = 40000;
    Timer_Config.counter_dir = TIMER_COUNT_UP;
    Timer_Config.intr_type = TIMER_INTR_LEVEL;
    timer_init(TIMER_GROUP_0, TIMER_0, &Timer_Config);

    ledc_timer_config_t PWM_Timer_Config;
	PWM_Timer_Config.clk_cfg = LEDC_USE_APB_CLK ;
	PWM_Timer_Config.duty_resolution = LEDC_TIMER_7_BIT ;
	PWM_Timer_Config.freq_hz = 40000;
	PWM_Timer_Config.speed_mode = LEDC_HIGH_SPEED_MODE;
	PWM_Timer_Config.timer_num = LEDC_TIMER_0;
	ledc_timer_config(&PWM_Timer_Config);

	ledc_channel_config_t PWM_Channel_Config;
	PWM_Channel_Config.channel = LEDC_CHANNEL_0;
	PWM_Channel_Config.duty = 30;
	PWM_Channel_Config.gpio_num = 17;
	PWM_Channel_Config.hpoint = 0;
	PWM_Channel_Config.intr_type = LEDC_INTR_DISABLE ;
	PWM_Channel_Config.speed_mode = LEDC_HIGH_SPEED_MODE ;
	PWM_Channel_Config.timer_sel = LEDC_TIMER_0;
	ledc_channel_config(&PWM_Channel_Config);


/*	adc_digi_config_t ADC_Digital_Init;
	ADC_Digital_Init.adc1_pattern =
	ADC_Digital_Init.adc1_pattern_len =
	ADC_Digital_Init.conv_limit_en =
	ADC_Digital_Init.conv_limit_num =
	ADC_Digital_Init.conv_mode = both ;
	ADC_Digital_Init.format =

	adc_digi_init();*/

	adc1_config_width(ADC_WIDTH_BIT_9);
	adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);

    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 20);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_isr_handler, NULL, 0, &s_timer_handle);

    timer_start(TIMER_GROUP_0, TIMER_0);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(PUSHBUTTON, gpio_isr_handler, (void*) PUSHBUTTON);

    Actual_time = 0;

//    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
//    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_9, 1100, adc_chars);

//    printf("VREF status: %u\n", val_type);

    while(1) {

    	Actual_time = esp_timer_get_time();
    	Actual_Timer_time = Int_counter;

    	for (i=0; i<5; i++)
		{
			gpio_set_level(GPIO_OUTPUT_LED, 0);
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
			ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
			vTaskDelay(10);

			gpio_set_level(GPIO_OUTPUT_LED, 1);
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 120);
			ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
			vTaskDelay(10);
		}

		gpio_set_level(GPIO_OUTPUT_LED, 1);
//		gpio_set_level(GPIO_OUTPUT_SPARE, 0);
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 1);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
		vTaskDelay(50);

		ADC_CH0_value = adc1_get_raw(ADC1_CHANNEL_0);

    	*((volatile uint32_t *) (0x3FF5F00C)) = 0x01 ;	// Catch Timer Group0 TMR0 counter value - TIMG0_T0UPDATE_REG

    	Timer_Low_value = *((volatile uint32_t *) (0x3FF5F004)) ;		// Read Timer Group0 TMR0 counter value - TIMG0_T0LO_REG value
    	Timer_High_value = *((volatile uint32_t *) (0x3FF5F008)) ;		// Read Timer Group0 TMR0 counter value - TIMG0_T0HI_REG value

    	GPIO_Status_main = GPIO.status;

    	test_method(Timer_Low_value, 0);
    	test_method(Timer_High_value, 1);
    	test_method(Elapsed_time, 2);
    	test_method(GPIO_Status_int, 3);
    	test_method(ADC_CH0_value, 4);

        Actual_time = esp_timer_get_time() - Actual_time ;
        Actual_time_int32 = (unsigned int)Actual_time ;
        Elapsed_time = Int_counter - Actual_Timer_time;

    }
}

void IRAM_ATTR gpio_isr_handler(void* arg)
{
	GPIO_Status_int = GPIO.status;
//	TIMERG0.hw_timer[0].
//	Int_counter++ ;
}

void IRAM_ATTR timer_isr_handler(void* arg)
{
    Int_counter++ ;
	TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;
}
