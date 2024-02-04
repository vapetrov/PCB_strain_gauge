#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "math.h"

#define ADC_ADDR 0b01101000
#define READ_SINGLE_1x 0b10001000
#define READ_SINGLE_8x 0b10001011

#define FILTER_SETTLE_MS 50
#define MAX_PWM_STEPS 4096

#define LED_G 16
#define LED_B 25
#define LED_R 17

void i2c_error(){

    printf("i2c has failed.\n");
    while(1);
}

int16_t read_adc(uint8_t config){

    uint8_t data_w = config;
    uint8_t data_r[3];
    int transfer;

    transfer = i2c_write_timeout_us(i2c1, ADC_ADDR, &data_w, 1, false, 1000 );
    if( transfer != 1 ) i2c_error();
    sleep_ms(50); // 11 < SPS < 20.5

    bool done;
    unsigned int loops = 0;
    do{
        transfer = i2c_read_timeout_us(i2c1, ADC_ADDR, data_r, 3, false, 1000); 
        if( transfer !=3 ) i2c_error();
        done = !(data_r[2]&(1<<7));
        if( !done )sleep_ms(5);
        loops++;
    }while( !done && loops <= 13);

    if( loops >= 13 ){
        i2c_error();
    }

    int16_t adc_read = __builtin_bswap16(*((int16_t*)data_r));
    return adc_read;
}



int main()
{

    stdio_init_all();
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_put(2, 1);
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_put(LED_R, 1);
    gpio_put(LED_G, 1);
    gpio_put(LED_B, 1);
    i2c_init(i2c1, 100E3);
    gpio_set_function(6, GPIO_FUNC_I2C);
    gpio_set_function(7, GPIO_FUNC_I2C);


    // wait for serial console.
    while(!stdio_usb_connected()); 
    gpio_put(LED_B, 0);
    printf("Starting...\n");


    // Find optimal PWM duty cycle with a binary search.
    // ~4000 steps is the highest we can achive while still being able to remove AC component with 2nd order LPF.

    int16_t adc_val;
    int16_t pwm_new;
    int16_t pwm_low = 0;
    int16_t pwm_high = MAX_PWM_STEPS;

    gpio_set_function(1, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(1);
    pwm_set_wrap(slice_num, pwm_high);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_num, true);

    while( pwm_high - pwm_low > 2 ){
        pwm_new = (pwm_high+pwm_low)/2;
        pwm_set_chan_level(slice_num, PWM_CHAN_B, pwm_new);
        sleep_ms(FILTER_SETTLE_MS);
        adc_val = read_adc(READ_SINGLE_8x);
        printf("%d/%d => %d\n", pwm_new, MAX_PWM_STEPS, adc_val);

        if(adc_val < 0)pwm_low = pwm_new;
        if(adc_val > 0)pwm_high = pwm_new;
        // TODO: off my one here.
        if(abs(adc_val) < 100)break;
    }

    // step through additional PWM frequencies to find a more optimal solution.

    float target_approx = pwm_new*1.0 / (MAX_PWM_STEPS);
    const int16_t primes[] = {2011, 2153, 2333, 2467, 2659, 2789, 2953, 3121, 3307, 3463, 3613, 3769, 3929, 4099};
    int16_t optimal_pwm = 0;
    int16_t optimal_div = 0;
    int32_t lowest_adc = 99999;
    for(int i=0; i<14; i++){
        pwm_new = lrint(target_approx * primes[i]);
        int16_t pwm_lowest = pwm_new; 
        pwm_set_wrap(slice_num, primes[i]);
        pwm_set_chan_level(slice_num, PWM_CHAN_B, pwm_new);
        sleep_ms(FILTER_SETTLE_MS);
        int32_t val = 0;
        val = read_adc(READ_SINGLE_8x);
        int32_t val_lowest = val;
        int dir = val_lowest < 0 ? 1 : -1 ;
        while(1){
            pwm_new = pwm_new + dir;
            pwm_set_chan_level(slice_num, PWM_CHAN_B, pwm_new);
            sleep_ms(FILTER_SETTLE_MS);
            int32_t val = 0;
            val = read_adc(READ_SINGLE_8x);
            if(abs(val) < abs(val_lowest)){
                val_lowest = val;
                pwm_lowest = pwm_new;
            }else{
                break;
            }
        }

        if(abs(val_lowest) < abs(lowest_adc)){
            lowest_adc = val_lowest;
            optimal_pwm = pwm_lowest;
            optimal_div = primes[i];
        }
        printf("%d/%d => %ld\n",pwm_lowest, primes[i], val_lowest);
    }

    // Any remaining bias will be subtracted digitally.

    pwm_set_wrap(slice_num, optimal_div);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, optimal_pwm);
    sleep_ms(FILTER_SETTLE_MS);
    int32_t bias = 0;
    for(int i=0;i<8;i++)bias = bias + read_adc(READ_SINGLE_8x);
    bias = bias/8;
    printf("Final optimization: %ld\n", bias);
    printf("============\n");
    gpio_put(LED_B, 1);

    int32_t window[32] = {0};
    uint16_t pos=0;
    while(1){

        // pick a gain level.

        //window[pos] = read_adc(READ_SINGLE_8x)-bias;
        window[pos] = read_adc(READ_SINGLE_1x)-(bias/8);

        pos = (pos+1)%32;
        gpio_put(LED_G, 1);
        if(pos%8 == 0){
            gpio_put(LED_G, 0);
            int32_t sum = 0;
            for(int i=0; i<32; i++){
                sum = sum+window[i];
            }
            sum = sum/32;
            printf("%ld\n",sum);
        }
    }


}

