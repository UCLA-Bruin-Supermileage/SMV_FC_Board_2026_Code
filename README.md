# SMV Front Control Board

This is the test bench for the front control board.

## Setup

This board depends on the [SMV CANbus library](https://github.com/UCLA-Bruin-Supermileage/SMV_STM32_CANbus) and the [SMV ADS1118 library](https://github.com/UCLA-Bruin-Supermileage/SMV_ADS1118).

### Pin Mappings

Peripheral control pins should be set to GPIO_Output.

| Name | Pin |
|----- | ---- |
| Wiper2 | PA2 |
| Wiper4 | PA3 |
| Horn | PA6 | 
| HLL | PA7 |
| HLR | PB6 |
| BlinkLeft | PA9 |
| BlinkRight | PC7 |
| CAN1_TX | PB9 |
| CAN1_RX | PB8 |
| CS | PD2 |
| SPI3_MOSI | PC12 |
| SPI3_MISO | PC11 |
| SPI3_SCK | PC10 |


## CAN Communication

### CAN TX

The FC board broadcasts the following sensor readings through the use of the ADS1118.

| Message | ADC Channel | Target Board | 
| ------- | ------------| ------------ |
| FC_Pressure | 0 | RC |
| Brake | 1 | MC |
| Gas | 2 | DAQ |

### CAN RX

The FC board listens for the following commands from the User Interface (UI) board to trigger these physical commands. A message of '1' is interpreted as HIGH and a message of '0' is interpreted as LOW.

| Message | Action |
| ------- | ------ |
| Horn | Set PA6 HIGH/LOW |
| Wipers | *Pending implementation* |
| Headlights | Set PA7 and PB6 HIGH/LOW |
| Blink_Left | Toggles PA9 at 500ms intervals when active |
| Blink_Right | Toggles PC7 at 500ms intervals when active |
