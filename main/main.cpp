#include <vector>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

#include "at24c.h"

extern "C"
{
    #include <stdio.h>
    #include <stdint.h>
    #include <string.h>
    
    #include "esp_err.h"
    #include "esp_vfs_dev.h"
    #include "driver/uart.h"
    #include "sdkconfig.h"
    
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    
    #include "driver/i2c.h"
    #include "esp_log.h"
    
    void app_main();
}

/////////////////////////////////

EEPROM_t dev;
esp_err_t ret;

const EEPROM_CONFIG_t config = {
    .data_gpio = 21,
    .clock_gpio= 22,
    .chip_type = _24C04
};

const uint16_t EEPROM_SIZE = config.chip_type*128;

uint8_t buf1[EEPROM_SIZE] = {};
uint8_t buf2[EEPROM_SIZE] = {};
uint8_t buf3[EEPROM_SIZE] = {};

/////////////////////////////////

esp_err_t configure_stdin_stdout()
{
    static bool configured = false;
    if (configured) return ESP_OK;
    // Initialize VFS & UART so we can use std::cout/cin
    setvbuf(stdin, NULL, _IONBF, 0);
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 256, 0, NULL, 0));
    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);
    configured = true; return ESP_OK;
}

vector<string> split_string(string& str, const char delim)
{
    string tok;
    vector<string> tokens;
    
    for (auto &ch : str)
    {
        if (ch == delim)
        {
            tokens.push_back(tok);
            tok.clear();
        }
        else
        {
            tok += ch;
        }
    }
    
    tokens.push_back(tok);
    return tokens;
}

uint8_t base_select_menu()
{
    uint base;

    cout << "Select Format :-\n\n"
         << "0  --> ASCII (output only)\n"
         << "10 --> INT ARRAY\n"
         << "16 --> HEX ARRAY\n\n";
    
    cout << "Enter Choice : "; scanf("%u", &base); printf("%u\n\n", base);

    if (base != 0 && base != 10 && base != 16)
    {
        puts("Invalid Input, Please Try Again !");
        return base_select_menu();
    }
    else
    {
        return base;
    }
}

void scan_data(uint8_t dt[], uint16_t len)
{
    char delim; string str;
    
    uint8_t format = base_select_menu();
    cout << "Enter Deliminator : "; scanf(" %c", &delim); printf("%c\n", delim);
    cout << "Enter Data :\n"; getline(cin, str); cout << str << endl;
    
    vector<uint8_t> bytes;
    vector<string> tokens = split_string(str, delim);
    
    for (auto &tok : tokens)
    {
        try
        {
            bytes.push_back(stoul(tok, 0, format));
        }
        catch (exception &e)
        {
            cout << e.what() << ", assuming 0" << endl;
            bytes.push_back(0);
        }
    }
    
    for (int i=0 ; i < len ; i++)
    {
        dt[i] = bytes[i];
    }
}

void print_data(uint8_t dt[], uint16_t len)
{
    uint8_t base = base_select_menu();
    
    switch (base)
    {
    case 0 :
        for (uint16_t i=0 ; i < len ; i++)
            printf("%c", dt[i]);
        break;
    
    case 10 :
        for (uint16_t i=0 ; i < len ; i++)
            printf("%03u ", dt[i]);
        break;
    
    case 16 :
        for (uint16_t i=0 ; i < len ; i++)
            printf("%02x ", dt[i]);
        break;
    
    default :
        puts("Unexpected Error Occured !!");
        return;
    }
    
    fflush(stdout);
}

void loadROM(uint8_t buf[], uint16_t len)
{
    for (int i=0 ; i < len ; i++)
    {
        ret = ReadRom(&dev, i, &buf[i]);
        if (ret != ESP_OK)
        {
            ESP_LOGE("loadROM", "ReadRom[%d] failed, this is most likely due to loose or incorrect wiring.", i);
            break;
        }
    }
}

void dumpROM(uint8_t buf[], uint16_t len)
{
    for (int i=0 ; i < len ; i++)
    {
        ret = WriteRom(&dev, i, buf[i]);
        if (ret != ESP_OK)
        {
            ESP_LOGE("dumpROM", "WriteRom[%d] failed, this is most likely due to loose or incorrect wiring.", i);
            break;
        }
    }
}

void clearROM(uint8_t buf[], uint16_t start, uint16_t end)
{
    for (int i=start ; i <= end ; i++)
    {
        buf[i] = 0;
    }
}

void verifyROM(uint8_t bf1[], uint8_t bf2[], uint16_t len)
{
    uint16_t err_count = 0;
    bool verify_failed = false;

    for (int i=0 ; i < len ; i++)
    {
        if (bf1[i] != bf2[i])
        {
            verify_failed = true; err_count += 1;
            printf("Memory Verification failed at address 0x%02x, %u != %u\n", i, bf1[i], bf2[i]);
        }
    }

    if (verify_failed)
        printf("Verification Failed with %u Errors !\n", err_count);
    else
        printf("Verified Successfully, No Errors !\n");
}

uint8_t* buffer_select_menu(uint8_t bn)
{
    unsigned selection;

    printf("Select Buffer %u :-\n", bn);
    puts("");
    puts("1 --> BUFFER 1");
    puts("2 --> BUFFER 2");
    puts("3 --> BUFFER 3");
    puts("");
    printf("Enter Selection : "); scanf("%u", &selection); printf("%u\n\n", selection);

    switch (selection)
    {
    case 1:
        return buf1;
    
    case 2:
        return buf2;
    
    case 3:
        return buf3;
    
    default:
        puts("Invalid Input, Please Try Again !");
        return buffer_select_menu(bn);
    }
}

void main_menu()
{
    unsigned selection;

    puts("\nAvailable Actions :-");
    puts("");
    puts("1 --> Load EEPROM to BUFFER");
    puts("2 --> Dump BUFFER to EEPROM");
    puts("3 --> Load STDIN to BUFFER");
    puts("4 --> Dump BUFFER to STDOUT");
    puts("5 --> Compare 2 BUFFERS");
    puts("6 --> Fill BUFFER with 0s");
    puts("0 --> Reset ESP32 & BUFFERS");
    puts("");
    printf("Choose Actions : "); scanf("%u", &selection); printf("%u\n\n", selection);

    switch (selection)
    {
    case 1:
        //Load EEPROM to BUFFER
        loadROM(buffer_select_menu(1), EEPROM_SIZE);
        break;
    
    case 2:
        //Dump BUFFER to EEPROM
        dumpROM(buffer_select_menu(1), EEPROM_SIZE);
        break;
    
    case 3:
        //Load STDIN to BUFFER
        scan_data(buffer_select_menu(1), EEPROM_SIZE);
        break;
    
    case 4:
        //Dump BUFFER to STDOUT
        print_data(buffer_select_menu(1), EEPROM_SIZE);
        break;
    
    case 5:
        //Compare 2 BUFFERS
        verifyROM(buffer_select_menu(1), buffer_select_menu(2), EEPROM_SIZE);
        break;
    
    case 6:
        //Fill BUFFER with 0s
        clearROM(buffer_select_menu(1), 1, EEPROM_SIZE);
        break;
    
    case 0:
        //Reset ESP32 & BUFFERS
        esp_restart(); break;
    
    default:
        puts("Invalid Input, Please Try Again !");
    }
}

void app_main()
{
    configure_stdin_stdout();
    InitRom(dev, config);
    while (true) main_menu();
}
