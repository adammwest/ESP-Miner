#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "esp_log.h"
#include "soc/uart_struct.h"

#include "bm1397.h"
#include "bm1368.h"
#include "serial.h"
#include "utils.h"

#define ECHO_TEST_TXD (17)
#define ECHO_TEST_RXD (18)
#define BUF_SIZE (1024)

static const char *TAG = "serial";

esp_err_t SERIAL_init(void)
{
    ESP_LOGI(TAG, "Initializing serial");
    // Configure UART1 parameters
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART1 parameters
    ESP_ERROR_CHECK_WITHOUT_ABORT(uart_param_config(UART_NUM_1, &uart_config));
    // Set UART1 pins(TX: IO17, RX: I018)
    ESP_ERROR_CHECK_WITHOUT_ABORT(uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Install UART driver (we don't need an event queue here)
    // tx buffer 0 so the tx time doesn't overlap with the job wait time
    //  by returning before the job is written
    return uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
}

esp_err_t SERIAL_set_baud(int baud)
{
    ESP_LOGI(TAG, "Changing UART baud to %i", baud);

    // Make sure that we are done writing before setting a new baudrate.
    ESP_ERROR_CHECK_WITHOUT_ABORT(uart_wait_tx_done(UART_NUM_1, 1000 / portTICK_PERIOD_MS));

    ESP_ERROR_CHECK_WITHOUT_ABORT(uart_set_baudrate(UART_NUM_1, baud));

    return ESP_OK;
}

int SERIAL_send(uint8_t *data, int len, bool debug)
{
    if (debug)
    {
        printf("tx: ");
        prettyHex((unsigned char *)data, len);
        printf("\n");
    }

    return uart_write_bytes(UART_NUM_1, (const char *)data, len);
}

/*
more advanced timeout rx function
*/
int uart_read_bytes_timeout(uart_port_t uart_num, void *buf, uint32_t length, float timeout_ms)  {
    //garuntees timeout waiting size 11 is the nonce message len
    //attampts to garuntees bytes read %11 = 0

    //we could use uart_get_buffered_data_len to get the length of the buffer
    //to get buf %11==0 only if there is enough time left

    int buf_idx = 0;
    uint64_t job_start = esp_timer_get_time();
    float time_left_ms = timeout_ms;

    while(true) {
        if (time_left_ms<0.01) break; //time limit
        if (buf_idx+1>=length) break; //size limit

        int recieved = uart_read_bytes(uart_num,buf+buf_idx,1, pdMS_TO_TICKS(time_left_ms));

        if (recieved<0) { 
            // if error add a small timeout
            vTaskDelay(pdMS_TO_TICKS(10));

            size_t len_test = 0;
            uart_get_buffered_data_len(UART_NUM_1, &len_test);
            if (len_test==0) SERIAL_clear_buffer();

        } else if (recieved>0) {
            buf_idx+=recieved;
        }

        uint64_t job_end = esp_timer_get_time();
        int64_t job_elapsed_us = job_end-job_start;
        float job_elapsed_ms = (float)(job_elapsed_us)/1000.0;
        time_left_ms = timeout_ms-job_elapsed_ms;
        
    }
    
    // now deal with the extra bytes that could be in buf
    size_t extra_len = 0;
    uart_get_buffered_data_len(UART_NUM_1, &extra_len);
    if (extra_len>0) {
        //get the buf length round down to 11 bytes
        int bytes_left = extra_len-(extra_len+buf_idx)%11;
        int recieved = uart_read_bytes(uart_num,buf+buf_idx,bytes_left, pdMS_TO_TICKS(10));
        buf_idx += recieved;
    } else {
        // here we have a x % 11 !=0 situation, uart_get_buffered_data_len has failed 
        // this is offset by some bytes we lose the last nonce
    }
    //add in serial debug 
    
    uint64_t job_end = esp_timer_get_time();
    int64_t job_elapsed_us = job_end-job_start;
    float job_elapsed_ms = (float)(job_elapsed_us)/1000.0;
    time_left_ms = timeout_ms-job_elapsed_ms;
    //ESP_LOGI(TAG, "buf %i time_left_ms %.3f/%.3f",buf_idx,time_left_ms,timeout_ms);

    //printf("rx: ");
    //prettyHex((unsigned char*) buf, buf_idx);
    return buf_idx;
}

/// @brief waits for a serial response from the device
/// @param buf buffer to read data into
/// @param buf number of ms to wait before timing out
/// @return number of bytes read, or -1 on error
int16_t SERIAL_rx(uint8_t *buf, uint16_t size, uint16_t timeout_ms)
{
    int16_t bytes_read = uart_read_bytes(UART_NUM_1, buf, size, timeout_ms / portTICK_PERIOD_MS);

    #if BM1937_SERIALRX_DEBUG || BM1366_SERIALRX_DEBUG || BM1368_SERIALRX_DEBUG
    size_t buff_len = 0;
    if (bytes_read > 0) {
        uart_get_buffered_data_len(UART_NUM_1, &buff_len);
        printf("rx: ");
        prettyHex((unsigned char*) buf, bytes_read);
        printf(" [%d]\n", buff_len);
    }
    #endif

    return bytes_read;
}

void SERIAL_debug_rx(void)
{
    int ret;
    uint8_t buf[100];

    ret = SERIAL_rx(buf, 100, 20);
    if (ret < 0)
    {
        fprintf(stderr, "unable to read data\n");
        return;
    }

    memset(buf, 0, 100);
}

void SERIAL_clear_buffer(void)
{
    uart_flush(UART_NUM_1);
}
