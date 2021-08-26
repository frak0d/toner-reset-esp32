#pragma once

// Modified version of https://github.com/nopnop2002/esp-idf-24c
// Driver for reading and writing data to 24Cxx external I2C EEPROMs.

#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern "C"
{
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    
    #include "driver/i2c.h"
    #include "esp_log.h"
}

#define AT24C_TAG "at24c"

#define ACK_CHECK_EN    0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS   0x0     /*!< I2C master will not check ack from slave */

struct EEPROM_t
{
    uint16_t _i2c_port;
    uint16_t _chip_addr;
    uint16_t _kbits;
    uint16_t _address;
};

enum chip_type_t : uint16_t
{
    _24C02  = 2,
    _24C04  = 4,
    _24C08  = 8,
    _24C16  = 16,
    _24C32  = 32,
    _24C64  = 64,
    _24C128 = 128,
    _24C256 = 256,
    _24C512 = 512,
};

struct EEPROM_CONFIG_t
{
    int data_gpio;
    int clock_gpio;
    chip_type_t chip_type;
    uint16_t i2c_port = I2C_NUM_0;
    uint16_t i2c_address = 0x50;
    uint32_t i2c_frequency = 100000;
};

esp_err_t InitRom(EEPROM_t& dev, const EEPROM_CONFIG_t& cfg)
{
    ESP_LOGI(AT24C_TAG, "EEPROM is 24C%.02d",cfg.chip_type);
    ESP_LOGI(AT24C_TAG, "CONFIG_SDA_GPIO=%d",cfg.data_gpio);
    ESP_LOGI(AT24C_TAG, "CONFIG_SCL_GPIO=%d",cfg.clock_gpio);
    ESP_LOGI(AT24C_TAG, "CONFIG_I2C_ADDRESS=0x%x",cfg.i2c_address);
    
    dev._i2c_port = cfg.i2c_port;
    dev._chip_addr = cfg.i2c_address;
    dev._kbits = cfg.chip_type;
    dev._address = (cfg.chip_type*128) - 1;
    
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = cfg.data_gpio;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = cfg.clock_gpio;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = cfg.i2c_frequency;
    
    esp_err_t ret = i2c_param_config(cfg.i2c_port, &conf);
    ESP_LOGD(AT24C_TAG, "i2c_param_config=%d", ret);
    if (ret != ESP_OK) return ret;
    ret = i2c_driver_install(cfg.i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    ESP_LOGD(AT24C_TAG, "i2c_driver_install=%d", ret);
    return ret;
}

static esp_err_t ReadReg8(EEPROM_t* dev, i2c_port_t i2c_port, int chip_addr, uint8_t data_addr, uint8_t * data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t WriteReg8(EEPROM_t* dev, i2c_port_t i2c_port, int chip_addr, uint8_t data_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    usleep(1000*2);
    return ret;
}

static esp_err_t ReadReg16(EEPROM_t* dev, i2c_port_t i2c_port, int chip_addr, uint16_t data_addr, uint8_t * data)
{
    uint8_t high_addr = (data_addr >> 8) & 0xff;
    uint8_t low_addr = data_addr & 0xff;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, high_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, low_addr, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t WriteReg16(EEPROM_t* dev, i2c_port_t i2c_port, int chip_addr, uint16_t data_addr, uint8_t data)
{
    uint8_t high_addr = (data_addr >> 8) & 0xff;
    uint8_t low_addr = data_addr & 0xff;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, high_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, low_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    usleep(1000*2);
    return ret;
}

esp_err_t ReadRom(EEPROM_t* dev, uint16_t data_addr, uint8_t * data)
{
    if (data_addr > dev->_address) return 0;

    if (dev->_kbits < 32) {
        int blockNumber = data_addr / 256;
        uint16_t _data_addr = data_addr - (blockNumber * 256);
        int _chip_addr = dev->_chip_addr + blockNumber;
        ESP_LOGD(AT24C_TAG, "ReadRom _chip_addr=%x _data_addr=%d", _chip_addr, _data_addr);
        return ReadReg8(dev, dev->_i2c_port, _chip_addr, _data_addr, data);
    } else {
        int _chip_addr = dev->_chip_addr;
        return ReadReg16(dev, dev->_i2c_port, _chip_addr, data_addr, data);
    }
}

esp_err_t WriteRom(EEPROM_t* dev, uint16_t data_addr, uint8_t data)
{
    if (data_addr > dev->_address) return 0;

    if (dev->_kbits < 32) {
        int blockNumber = data_addr / 256;
        uint16_t _data_addr = data_addr - (blockNumber * 256);
        int _chip_addr = dev->_chip_addr + blockNumber;
        ESP_LOGD(AT24C_TAG, "WriteRom _chip_addr=%x _data_addr=%d", _chip_addr, _data_addr);
        return WriteReg8(dev, dev->_i2c_port, _chip_addr, _data_addr, data);
    } else {
        int _chip_addr = dev->_chip_addr;
        return WriteReg16(dev, dev->_i2c_port, _chip_addr, data_addr, data); 
    }
}

uint16_t MaxAddress(EEPROM_t* dev)
{
    return dev->_address;
}
