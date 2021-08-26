# toner-reset-esp32
esp32 program to reprogram old samsung toner chip over i2c

### How this Works ?
The printer stores the page count inside the EEPROM chip of Toner/Cartridge.
Using this Program you can read the contents of the chip, copy it to your PC, edit it and then flash back to the chip, all via esp32.
If unsure what to edit, you could start by reading data from new toner and writing it on used toner and changing serial numbers etc.

### How to Use ?
1. Clone this repository locally.
2. Set `config.chip_type` in `main/main.cpp` according to your toner's flash chip.
3. Open ESP-IDF shell, cd to the folder made by git.
4. Connect the board to PC, then flash using `idf.py build flash monitor`.
5. Connect the toner chip to esp32 with CLOCK on pin 22 and DATA on pin 21, along with 3v3 and GND.
