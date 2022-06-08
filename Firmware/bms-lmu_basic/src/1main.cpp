// #include <Arduino.h>
// #include "bms_lmu.h"
// #include "SPI.h"
// #include <eXoCAN.h>
// #include "TickerInterrupt.h"
// #include "PBalancer.h"

// /******************************************************************************
//  * BMS_LMU - HARDWARE REVISION 0
//  * Patrick Curtain & Ethan Suter
//  * 
//  * Battery Management System - Local Measurement Unit
//  * 
//  * An active balancer created for FYRP by Patrick Curtain and Ethan Suter, 
//  * Semester 2 2021 and Semester 1 2022, at Swinburne University of Technology.
//  * 
//  * Code base derived from Team Swinburne FSAE
//  * Available: https://github.com/Team-Swinburne/ts_20-code-development
//  * 
//  * Revision     Date      Comments
//  * --------   --------    --------
//  * 0.0        28/5/2022   Initial Code Base
//  * 
//  * 
// ******************************************************************************/

// #define HW_REV "0"
// #define SW_REV "0.0"

// // PINOUT
// #define LED0 PC13
// #define CAN_TX_LED PB5
// #define CAN_RX_LED PB6
// #define SPI_TX_LED PB7
// #define SPI_RX_LED PB8
// #define FAULT_RELAY PA9
// #define FAULT_RELAY_FEEDBACK PA10

// // NOT SAFETY PARAMETERS
// #define DISCHARGE_TIMER_LIMIT 1000
// #define HEART_RATE 1000
// #define CAN_INTERVAL 800
// #define CANBUS_FREQUENCY 250000

// #define TX_ADDRESS 0x481

// // led pins
// DigitalOut can_tx_led(CAN_TX_LED);
// DigitalOut can_rx_led(CAN_TX_LED);
// DigitalOut spi_tx_led(CAN_TX_LED);
// DigitalOut spi_rx_led(CAN_TX_LED);

// // Globals
// Heartbeat heartbeat(FAULT_RELAY, FAULT_RELAY_FEEDBACK, LED0);
// Stack stack;
// PBalancer passive_balancer(stack, DISCHARGE_TIMER_LIMIT);

// // Interfaces
// eXoCAN can;

// // Timers
// TickerInterrupt ticker(TIM2, 1);

// struct msg_frame
// {
//   uint8_t len = 8;
//   uint8_t bytes[8] = {0};
// };

// static msg_frame	heart_frame {.len = 3},
//                	bms_lower_bank {.len = 6}, 
// 				        bms_upper_bank {.len = 6},
//                	temperature {.len = 8};

// uint8_t rxData[8];

// void can_tx(){
//   can.transmit(TX_ADDRESS, heart_frame.bytes, heart_frame.len);
//   can.transmit(TX_ADDRESS + 1, bms_lower_bank.bytes, bms_lower_bank.len);
//   can.transmit(TX_ADDRESS + 2, bms_upper_bank.bytes, bms_upper_bank.len);
//   can.transmit(TX_ADDRESS + 3, temperature.bytes, temperature.len);

//   can_tx_led = !can_tx_led;
// }

// // Can receive interupt service routine
// void canISR() {
// 	can.rxMsgLen = can.receive(can.id, can.fltIdx, can.rxData.bytes);
// }


// void can_rx() {
// 	if (can.rxMsgLen > -1) {
//     can_rx_led = !can_rx_led;
// 	}
// }

// uint8_t array_to_uint8(bool arr[], int count){
//     int ret = 0;
//     int tmp;
//     for (int i = 0; i < count; i++) {
//         tmp = arr[i];
//         ret |= tmp << (count - i - 1);
//     }
//     return ret;
// }

// uint16_t check_errors(){
//   bool error_code[8];

// 	error_code[ERROR_OV_FAULT] = stack.ov_fault();
// 	error_code[ERROR_UV_FAULT] = stack.uv_fault();
// 	// error_code[ERROR_OT_FAULT] = !imd.get_IMD_ok();
// 	error_code[ERROR_RELAY_FAULT] = heartbeat.relay_fault();
	
// 	// error_code[ERROR_ORION_LOW_VOTLAGE] 	= orion.check_low_voltage();
// 	// error_code[ERROR_ORION_HIGH_VOLTAGE] 	= orion.check_high_voltage();
// 	// error_code[ERROR_ORION_OVERTEMPERATURE] = orion.check_overtemperature();
// 	// error_code[ERROR_PERIPHERALS] = 0;

// 	return array_to_uint8(error_code, 8);
// }

// void update_can_frames(){
//   heart_frame.bytes[0] = heartbeat.state();
//   heart_frame.bytes[0] = heartbeat.counter();
//   heart_frame.bytes[1] = heartbeat.fault_code();

//   for (int i = 0; i < 6; i++){
//     bms_lower_bank.bytes[i] = stack.cell_voltage(i);
//   }

//   for (int i = 6; i < 12; i++){
//     bms_upper_bank.bytes[i] = stack.cell_voltage(i);
//   }

//   // add in the temp measurement thing.
// }

// void state_d(){

//   update_can_frames();
//   heartbeat.fault_code(check_error());

//   switch(heartbeat.state()) {
    
//     case (FAULT):
//       // do nothing, clear the error as quickly
//       // as possible.
//       if (check_errors() == 0){
//         heartbeat.state(IDLE);
//       }
//       break;
    
//     case (IDLE):
//       // literally do nothing!
//       break;
    
//     case (ACTIVE_BALANCING):
//       // not sure yet!
//       // push into passive state once done
//       break;
    
//     case (PASSIVE_BALANCING):
//       passive_balancer.start_balance();
//       break;
    
//   }
// }

// void start_up_lights(){
//   pinMode(PB5, OUTPUT);
//   pinMode(PB6, OUTPUT);
//   pinMode(PB7, OUTPUT);
//   pinMode(PB8, OUTPUT);

//   int scroller_time = 100;
//   digitalWrite(PB5, HIGH);
//   delay(scroller_time);
//   digitalWrite(PB6, HIGH);
//   delay(scroller_time);
//   digitalWrite(PB7, HIGH);
//   delay(scroller_time);
//   digitalWrite(PB8, HIGH);
//   delay(scroller_time);

//   digitalWrite(PB5, LOW);
//   delay(scroller_time);
//   digitalWrite(PB6, LOW);
//   delay(scroller_time);
//   digitalWrite(PB7, LOW);
//   delay(scroller_time);
//   digitalWrite(PB8, LOW);
//   delay(scroller_time);
// }


// // timer functions
// void heartbeat_cb(){
//   heartbeat.tick(check_errors());
// }

// void setup() {

//   // setup the serial.
//   Serial.begin(9600);
//   Serial.print("Starting BMS-LMU \n");
//   Serial.print("  HARDWARE REVISION - 0\n");
//   Serial.print("  SOFTWARE REVISION - 0.0\n\n");
//   Serial.print("Swinburne University of Technology - FYRP40002\n  Patrick Curtain \& Ethan Suter\n");

//   // setup the can
//   can.begin(STD_ID_LEN, CANBUS_FREQUENCY, PORTA_11_12_WIRE_PULLUP);   //11 Bit Id, 500Kbps
//   // can.filterMask16Init(0, 0x600, 0x7ff);
//   can.attachInterrupt(canISR);
  
//   // setup heartbeat tickers.
//   ticker.start();
//   ticker.attach(heartbeat_cb, HEART_RATE);

//   // start up the pbalancer
//   passive_balancer.setup();

//   ticker.attach(can_tx, CAN_INTERVAL);

//   // finished boot, flash the lights to confirm startup!
//   start_up_lights();
//   Serial.print("Finished boot! Starting\n");

//   Serial.begin(9600);
    
//     #define adSS PA0
//   pinMode(adSS, OUTPUT);
//   digitalWrite(adSS, HIGH);

//   SPI.begin();
// }

// DigitalOut temp_sensor_ss(PA0);
// DigitalOut active_balance_upper_ss(PB0);
// DigitalOut active_balance_lower_ss(PB4);
// DigitalOut passive_balance_ss(PA1);


// void loop() {

//     temp_sensor_ss = 0;
//     active_balance_upper_ss = 1;
//     active_balance_lower_ss = 1;
//     passive_balance_ss = 1;

// byte commandByte;
//   byte result[2];
  
//   for(byte x=0;x<8;x++) {
//     // set to adc# and bit3 HIGH
//     commandByte = x + 8;
//     // shift into position
//     commandByte = commandByte << 4;
//     // set low 4 bits HIGH
//     commandByte = commandByte | 0x0f;   

//     // enable ADC SPI slave select
//     digitalWrite(adSS, LOW);
//     delayMicroseconds(1);

//     SPI.transfer(commandByte);
//     // you might need to increase this delay for conversion
//     delayMicroseconds(2);

//     // get results
//     result[0] = SPI.transfer(0);
//     result[1] = SPI.transfer(0);

//     // disable ADC SPI slave select
//     digitalWrite(adSS,HIGH);    

//     Serial.print("AD");
//     Serial.print(x);
//     Serial.print(" = ");
//     Serial.print(result[0]);
//     Serial.print(" ");
//     Serial.println(result[1]);
//     delay(100);
//     ///
//   }
//   delay(1000);

// //   // state_d();
// //   delay(5000);
// //   passive_balancer.read_config();
// //   Serial.println(passive_balancer.get_errors());

// //   delay(2000);
// //   passive_balancer.read_cell_voltage_registers();
// }