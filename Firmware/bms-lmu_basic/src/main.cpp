/*! Analog Devices DC2259A Demonstration Board. 
* LTC6811: Multicell Battery Monitors
*
*@verbatim
*NOTES
* Setup:
*   Set the terminal baud rate to 115200 and select the newline terminator.
*   Ensure all jumpers on the demo board are installed in their default positions from the factory.
*   Refer to Demo Manual.
*
*USER INPUT DATA FORMAT:
* decimal : 1024
* hex     : 0x400
* octal   : 02000  (leading 0)
* binary  : B10000000000
* float   : 1024.0
*@endverbatim
*
* https://www.analog.com/en/products/ltc6811-1.html
* https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/dc2259a.html
*
********************************************************************************
* Copyright 2019(c) Analog Devices, Inc.
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  - Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  - Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
*  - Neither the name of Analog Devices, Inc. nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*  - The use of this software may or may not infringe the patent rights
*    of one or more patent holders.  This license does not release you
*    from the requirement that you obtain separate licenses from these
*    patent holders to use this software.
*  - Use of the software either in source or binary form, must be run
*    on or directly connected to an Analog Devices Inc. component.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/*! @file
    @ingroup LTC6811-1
*/

/*
NOTES
 Setup:
   Set the terminal baud rate to 115200 and select the newline terminator.
   Ensure all jumpers on the demo board are installed in their default positions from the factory.
   Refer to Demo Manual DC2259.

USER INPUT DATA FORMAT:
 decimal : 1024
 hex     : 0x400
 octal   : 02000  (leading 0)
 binary  : B10000000000
 float   : 1024.0
 */

#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "LT_SPI.h"
//#include "LT_I2C.h"           // Hardware I2C driver
//#include "QuikEval_EEPROM.h"
#include "UserInterface.h"   // serial interface routines to communicate with the user
#include "LTC681x.h"
#include "LTC6811.h"
#include <SPI.h>

#define ENABLED 1
#define DISABLED 0

#define DATALOG_ENABLED 1
#define DATALOG_DISABLED 0

void print_menu();
void print_cells(uint8_t datalog_en);
void print_aux(uint8_t datalog_en);
void print_stat();
void print_open();
void print_config();
void print_rxconfig();
void print_comm();
void print_rxcomm();
void print_pwmconfig();
void print_rxpwmconfig();
void print_sctrl();
void print_rxsctrl(); 
void print_statsoc();
// void print_aux1();
void print_aux1(uint8_t datalog_en);
void check_error(int error);
void print_pec();
void serial_print_hex(uint8_t data);
char get_char();
void run_command(uint32_t cmd);
void measurement_loop(uint8_t datalog_en);

/**********************************************************
  Setup Variables
  The following variables can be modified to
  configure the software.
***********************************************************/
const uint8_t TOTAL_IC = 1;//!< Number of ICs in the daisy chain

//ADC Command Configurations. See LTC681x.h for options.
const uint8_t ADC_OPT = ADC_OPT_DISABLED; //!< ADC Mode option bit
const uint8_t ADC_CONVERSION_MODE = MD_7KHZ_3KHZ; //!< ADC Mode
const uint8_t ADC_DCP = DCP_ENABLED; //!< Discharge Permitted 
const uint8_t CELL_CH_TO_CONVERT = CELL_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t AUX_CH_TO_CONVERT = AUX_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t STAT_CH_TO_CONVERT = STAT_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t NO_OF_REG = REG_ALL; //!< Register Selection
const uint16_t MEASUREMENT_LOOP_TIME = 500; //!< Loop Time in milliseconds(ms)

//Under Voltage and Over Voltage Thresholds
const uint16_t OV_THRESHOLD = 41000; //!< Over voltage threshold ADC Code. LSB = 0.0001 ---(4.1V)
const uint16_t UV_THRESHOLD = 30000; //!< Under voltage threshold ADC Code. LSB = 0.0001 ---(3V)

//Loop Measurement Setup. These Variables are ENABLED or DISABLED. Remember ALL CAPS
const uint8_t WRITE_CONFIG = ENABLED; //!< Loop Measurement Setup
const uint8_t READ_CONFIG = ENABLED; //!< Loop Measurement Setup
const uint8_t MEASURE_CELL = ENABLED; //!< Loop Measurement Setup
const uint8_t MEASURE_AUX = ENABLED; //!< Loop Measurement Setup
const uint8_t MEASURE_STAT = ENABLED; //!< Loop Measurement Setup
const uint8_t PRINT_PEC = ENABLED; //!< Loop Measurement Setup
/************************************
  END SETUP
*************************************/

/******************************************************
 Global Battery Variables received from 681x commands.
 These variables store the results from the LTC6811
 register reads and the array lengths must be based
 on the number of ICs on the stack
 ******************************************************/
cell_asic bms_ic[TOTAL_IC]; //!< Global Battery Variable

/*********************************************************
 Set the configuration bits. 
 Refer to the Configuration Register Group from data sheet. 
**********************************************************/
bool REFON = true; //!< Reference Powered Up Bit
bool ADCOPT = false; //!< ADC Mode option bit
bool gpioBits_a[5] = {false,false,false,false,false}; //!< GPIO Pin Control // Gpio 1,2,3,4,5
uint16_t UV=UV_THRESHOLD; //!< Under-voltage Comparison Voltage
uint16_t OV=OV_THRESHOLD; //!< Over-voltage Comparison Voltage
bool dccBits_a[12] = {false,false,false,false,false,false,false,false,false,false,false,false}; //!< Discharge cell switch //Dcc 1,2,3,4,5,6,7,8,9,10,11,12
bool dctoBits[4] = {true, false, true, false}; //!< Discharge time value // Dcto 0,1,2,3 // Programed for 4 min 
/*Ensure that Dcto bits are set according to the required discharge time. Refer to the data sheet */

#include "pin_abstraction.h"
DigitalOut temp_sensor_ss(PA0);
DigitalOut active_balance_upper_ss(PB0);
DigitalOut active_balance_lower_ss(PB4);
DigitalOut led0(PC13);

/*!**********************************************************************
 \brief  Initializes hardware and variables
 @return void
 ***********************************************************************/
void setup()
{
  temp_sensor_ss = 0;
  active_balance_upper_ss = 0;
  active_balance_lower_ss = 0;
  led0 = 0;
  delay(1000);
  led0 = 1;

  Serial.begin(9600);
  // quikeval_SPI_connect();
  
  spi_enable(SPI_CLOCK_DIV128); // This will set the Linduino to have a 1MHz Clock
  LTC6811_init_cfg(TOTAL_IC, bms_ic);
  for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
  {
    LTC6811_set_cfgr(current_ic,bms_ic,REFON,ADCOPT,gpioBits_a,dccBits_a, dctoBits, UV, OV);
  }
  LTC6811_reset_crc_count(TOTAL_IC,bms_ic);
  LTC6811_init_reg_limits(TOTAL_IC,bms_ic);
  print_menu();
}

/*!*********************************************************************
 \brief Main loop
 @return void
***********************************************************************/
void loop()
{
  if (Serial.available())           // Check for user input
  {
    uint32_t user_command;
    user_command = read_int();      // Read the user command
    if(user_command=='m')
    { 
      print_menu();
    }
    else
    { 
      Serial.println(user_command);
      run_command(user_command); 
    }   
  }
}


/*!*****************************************
 \brief Executes the user command
 @return void
*******************************************/
void run_command(uint32_t cmd)
{
  const uint8_t STREG=0;
  int8_t error = 0;
  uint32_t conv_time = 0;
  uint32_t user_command;
  int8_t readIC=0;
  char input = 0;
  
  switch (cmd)
  {
    case 1: // Write and Read Configuration Register
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,bms_ic);
      print_config();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,bms_ic);
      check_error(error);
      print_rxconfig();
      break;

    case 2: // Read Configuration Register
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,bms_ic);
      check_error(error);
      print_rxconfig();
      break;

    case 3: // Start Cell ADC Measurement
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      Serial.print(F("Cell conversion completed in:"));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F("ms"));
      Serial.println();
      break;

    case 4: // Read Cell Voltage Registers
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdcv(NO_OF_REG, TOTAL_IC,bms_ic); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);
      break;

    case 5: // Start GPIO ADC Measurement
      wakeup_sleep(TOTAL_IC);
      LTC6811_adax(ADC_CONVERSION_MODE, AUX_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      Serial.print(F("Aux conversion completed in: "));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F(" ms"));    
      Serial.println(); 
      break;

    case 6: // Read AUX Voltage Registers
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdaux(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all aux registers
      check_error(error);
      print_aux(DATALOG_DISABLED);
      break;

    case 7: // Start Status ADC Measurement
      wakeup_sleep(TOTAL_IC);
      LTC6811_adstat(ADC_CONVERSION_MODE, STAT_CH_TO_CONVERT);
      conv_time=LTC6811_pollAdc();
      Serial.print(F("Stat conversion completed in "));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F(" ms"));
      Serial.println();
      break;

    case 8: // Read Status registers
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdstat(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all aux registers
      check_error(error);
      print_stat();
      break;

    case 9:// Start Combined Cell Voltage and GPIO1, GPIO2 Conversion and Poll Status
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcvax(ADC_CONVERSION_MODE,ADC_DCP);
      conv_time = LTC6811_pollAdc();
      Serial.println(F("Start Combined Cell Voltage and GPIO1, GPIO2 conversion completed"));
      Serial.print(F("conversion completed in:"));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F("ms"));
      Serial.println();
      wakeup_idle(TOTAL_IC);
      error =LTC6811_rdcv(NO_OF_REG, TOTAL_IC,bms_ic); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);     
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdaux(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all aux registers
      check_error(error);
      print_aux1(DATALOG_DISABLED);
      Serial.println();
      break;
      
    case 10: //Start Combined Cell Voltage and Sum of cells
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcvsc(ADC_CONVERSION_MODE,ADC_DCP);
      conv_time = LTC6811_pollAdc();
      Serial.print(F("Combined Cell Voltage conversion completed in:"));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F("ms"));
      Serial.println();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcv(NO_OF_REG, TOTAL_IC,bms_ic); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdstat(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all stat registers
      check_error(error);
      print_statsoc();
      Serial.println();
      break;
      
    case 11: // Loop Measurements without data-log output
      Serial.println(F("transmit 'm' to quit"));
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,bms_ic);
      while (input != 'm')
      {
        if (Serial.available() > 0)
        {
          input = read_char();
        }

        measurement_loop(DATALOG_DISABLED);

        delay(MEASUREMENT_LOOP_TIME);
      }
      print_menu();
      break;

    case 12: //Data-log print option Loop Measurements
      Serial.println(F("transmit 'm' to quit"));
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,bms_ic);
      while (input != 'm')
      {
        if (Serial.available() > 0)
        {
          input = read_char();
        }

        measurement_loop(DATALOG_ENABLED);

        delay(MEASUREMENT_LOOP_TIME);
      }
      print_menu();
      break;

    case 13: // Run the Mux Decoder Self Test
      wakeup_sleep(TOTAL_IC);
      LTC6811_diagn();
      conv_time = LTC6811_pollAdc();
      Serial.print(F("DIAGN  conversion completed in: "));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F(" ms"));    
      Serial.println(); 
      error = LTC6811_rdstat(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all aux registers
      check_error(error);
      for (int ic = 0; ic<TOTAL_IC; ic++)
        { error = 0;
          Serial.print(" IC ");
          Serial.println(ic+1,DEC);
          if (bms_ic[ic].stat.mux_fail[0] != 0) error++;
        
          if (error==0) Serial.println(F("Mux Test: PASS "));
          else Serial.println(F("Mux Test: FAIL "));
        }
      Serial.println("");  
      break;

    case 14:  // Run the ADC/Memory Self Test
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_cell_adc_st(CELL,TOTAL_IC,bms_ic, ADC_CONVERSION_MODE, ADCOPT);
      Serial.print(error, DEC);
      Serial.println(F(" : errors detected in Digital Filter and CELL Memory \n"));

      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_cell_adc_st(AUX,TOTAL_IC, bms_ic, ADC_CONVERSION_MODE, ADCOPT);
      Serial.print(error, DEC);
      Serial.println(F(" : errors detected in Digital Filter and AUX Memory \n"));

      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_cell_adc_st(STAT,TOTAL_IC, bms_ic, ADC_CONVERSION_MODE, ADCOPT);
      Serial.print(error, DEC);
      Serial.println(F(" : errors detected in Digital Filter and STAT Memory \n"));
      print_menu();
      break;
      
    case 15: // Run ADC Overlap self test
      wakeup_sleep(TOTAL_IC);
      error = (int8_t)LTC6811_run_adc_overlap(TOTAL_IC,bms_ic);
      if (error==0) Serial.println(F("Overlap Test: PASS "));
      else Serial.println(F("Overlap Test: FAIL"));
      Serial.println(); 
      break;

    case 16: // Run ADC Redundancy self test
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_adc_redundancy_st(ADC_CONVERSION_MODE,AUX,TOTAL_IC, bms_ic);
      Serial.print(error, DEC);
      Serial.println(F(" : errors detected in AUX Measurement \n"));

      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_adc_redundancy_st(ADC_CONVERSION_MODE,STAT,TOTAL_IC, bms_ic);
      Serial.print(error, DEC);
      Serial.println(F(" : errors detected in STAT Measurement \n"));
      break;

    case 17: // Open Wire test for single cell detection
      wakeup_sleep(TOTAL_IC);         
      LTC6811_run_openwire_single(TOTAL_IC, bms_ic);
      print_open();
      Serial.println();    
      break;

    case 18: // Open Wire test for multiple cell and two consecutive cells detection
      wakeup_sleep(TOTAL_IC);         
      LTC6811_run_openwire_multi(TOTAL_IC, bms_ic);
      Serial.println();    
      break;

    case 19:// PEC Errors Detected
      print_pec(); 
      Serial.println();     
      break;

    case 20: //Reset PEC Counter
      LTC6811_reset_crc_count(TOTAL_IC,bms_ic);
      Serial.println(F("PEC counter reset"));
      Serial.println();
      break;
      
    case 21: // Enable a discharge transistor
      Serial.println(F("Please enter the Spin number:"));
      readIC = (int8_t)read_int();
      Serial.println(readIC);
      wakeup_sleep(TOTAL_IC);
      LTC6811_set_discharge(readIC,TOTAL_IC,bms_ic);
      LTC6811_wrcfg(TOTAL_IC,bms_ic);   
      print_config();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,bms_ic);
      check_error(error);
      print_rxconfig();
      break;
      
    case 22: // Clear all discharge transistors
      wakeup_sleep(TOTAL_IC);
      LTC6811_clear_discharge(TOTAL_IC,bms_ic);
      LTC6811_wrcfg(TOTAL_IC,bms_ic);    
      print_config();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,bms_ic);
      check_error(error);
      print_rxconfig();
      break;

    case 23://Write read pwm configuration     
      /*****************************************************
         PWM configuration data.
         1)Set the corresponding DCC bit to one for pwm operation. 
         2)Set the DCTO bits to the required discharge time.
         3)Choose the value to be configured depending on the
          required duty cycle. 
         Refer to the data sheet. 
      *******************************************************/ 
      wakeup_sleep(TOTAL_IC);
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        bms_ic[current_ic].pwm.tx_data[0]= 0x88; // Duty cycle for S pin 2 and 1
        bms_ic[current_ic].pwm.tx_data[1]= 0x88; // Duty cycle for S pin 4 and 3
        bms_ic[current_ic].pwm.tx_data[2]= 0x88; // Duty cycle for S pin 6 and 5
        bms_ic[current_ic].pwm.tx_data[3]= 0x88; // Duty cycle for S pin 8 and 7
        bms_ic[current_ic].pwm.tx_data[4]= 0x88; // Duty cycle for S pin 10 and 9
        bms_ic[current_ic].pwm.tx_data[5]= 0x88; // Duty cycle for S pin 12 and 11
      }          
      LTC6811_wrpwm(TOTAL_IC,0,bms_ic);
      print_pwmconfig(); 

      wakeup_idle(TOTAL_IC);
      LTC6811_rdpwm(TOTAL_IC,0,bms_ic);       
      print_rxpwmconfig();                              
      break;

    case 24: // Write and read S Control Register Group
      wakeup_sleep(TOTAL_IC);
      /**************************************************************************************
         S pin control. 
         1)Ensure that the pwm is set according to the requirement using the previous case.
         2)Choose the value depending on the required number of pulses on S pin. 
         Refer to the data sheet. 
      ***************************************************************************************/
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        bms_ic[current_ic].sctrl.tx_data[0]=0xFF; // No. of high pulses on S pin 2 and 1
        bms_ic[current_ic].sctrl.tx_data[1]=0xFF; // No. of high pulses on S pin 4 and 3
        bms_ic[current_ic].sctrl.tx_data[2]=0xFF; // No. of high pulses on S pin 6 and 5
        bms_ic[current_ic].sctrl.tx_data[3]=0xFF; // No. of high pulses on S pin 8 and 7
        bms_ic[current_ic].sctrl.tx_data[4]=0xFF; // No. of high pulses on S pin 10 and 9
        bms_ic[current_ic].sctrl.tx_data[5]=0xFF; // No. of high pulses on S pin 12 and 11
      }
      LTC6811_wrsctrl(TOTAL_IC,STREG,bms_ic);
      print_sctrl();

      // Start S Control pulsing
      wakeup_idle(TOTAL_IC);
      LTC6811_stsctrl();
      LTC6811_pollAdc();
      Serial.println(F("Start S Control Pulsing"));
      Serial.println();

      // Read S Control Register Group 
      wakeup_idle(TOTAL_IC);
      error=LTC6811_rdsctrl(TOTAL_IC,STREG,bms_ic);
      check_error(error);
      print_rxsctrl();
      break;

    case 25: // Clear S Control Register Group
      wakeup_sleep(TOTAL_IC);
      LTC6811_clrsctrl();
      Serial.println(F("S Control Register Cleared"));
      Serial.println(); 
      
      // Read S Control Register Group
      wakeup_idle(TOTAL_IC);
      error=LTC6811_rdsctrl(TOTAL_IC,STREG,bms_ic);
      check_error(error);
      print_rxsctrl();
      break;

    // case 26://Start SPI Communication on the GPIO Ports
    //   wakeup_sleep(TOTAL_IC);
    //   /************************************************************
    //      Communication control bits and communication data bytes. 
    //      Refer to the data sheet. 
    //   *************************************************************/  
    //    for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
    //   {                        
    //     bms_ic[current_ic].com.tx_data[0]= 0x80;
    //     bms_ic[current_ic].com.tx_data[1]= 0x00;
    //     bms_ic[current_ic].com.tx_data[2]= 0xA2;
    //     bms_ic[current_ic].com.tx_data[3]= 0x20;
    //     bms_ic[current_ic].com.tx_data[4]= 0xA3;
    //     bms_ic[current_ic].com.tx_data[5]= 0x39;
    //   }    
    //   LTC6811_wrcomm(TOTAL_IC,bms_ic);               
    //   print_comm();   

    //   wakeup_idle(TOTAL_IC);
    //   LTC6811_stcomm(); 
    //   LTC6811_pollAdc();           
    //   Serial.println(F("Stcomm SPI Communication completed"));
    //   Serial.println();  

    //   wakeup_idle(TOTAL_IC);
    //   error = LTC6811_rdcomm(TOTAL_IC,bms_ic);                       
    //   check_error(error);
    //   print_rxcomm();        
    //   break;

    // case 27: // write byte I2C Communication on the GPIO Ports(using eeprom 24AA01)
    //   wakeup_sleep(TOTAL_IC);
    //   /************************************************************
    //      Communication control bits and communication data bytes. 
    //      Refer to the data sheet. 
    //   *************************************************************/ 
    //   for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
    //   {
    //     bms_ic[current_ic].com.tx_data[0]= 0x6A; // Icom(6)Start + I2C_address D0 (1010 0000)
    //     bms_ic[current_ic].com.tx_data[1]= 0x00; // Fcom master ack  
    //     bms_ic[current_ic].com.tx_data[2]= 0x00; // eeprom address D1 (0000 0000)
    //     bms_ic[current_ic].com.tx_data[3]= 0x00; // Fcom master ack 
    //     bms_ic[current_ic].com.tx_data[4]= 0x01; // Icom BLANCK D2 (0Xxx)
    //     bms_ic[current_ic].com.tx_data[5]= 0x29; // Fcom master nack+stop 
    //   }             
             
    //   LTC6811_wrcomm(TOTAL_IC,bms_ic);// write comm register    
    //   print_comm();

    //   wakeup_idle(TOTAL_IC); 
    //   LTC6811_stcomm();// start I2C to write data into slave device
    //   LTC6811_pollAdc();
    //   Serial.println(F("Stcomm I2C Communication completed"));
    //   Serial.println(); 
    
    //   wakeup_idle(TOTAL_IC); 
    //   error = LTC6811_rdcomm(TOTAL_IC,bms_ic); // Read comm register                       
    //   check_error(error);
    //   print_rxcomm(); // Print comm register  
    //   break;
      
    // case 28: // Read byte data I2C Communication on the GPIO Ports(using eeprom 24AA01)
    //   wakeup_sleep(TOTAL_IC);
    //   /************************************************************
    //      Communication control bits and communication data bytes. 
    //      Refer to the data sheet. 
    //     *************************************************************/         
    //   for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
    //   {        
    //     bms_ic[current_ic].com.tx_data[0]= 0x6A; // Icom(6)Start + I2C_address D0 (1010 0000)
    //     bms_ic[current_ic].com.tx_data[1]= 0x10; // Fcom master ack 
    //     bms_ic[current_ic].com.tx_data[2]= 0x60; // again start I2C with I2C_address (1010 0001)
    //     bms_ic[current_ic].com.tx_data[3]= 0x19; // fcom master nack + stop  
    //   }            
    //   LTC6811_wrcomm(TOTAL_IC,bms_ic);

    //   wakeup_idle(TOTAL_IC); 
    //   LTC6811_stcomm();// I2C for write data in slave device 
    //   LTC6811_pollAdc();   
    //   Serial.println(F("I2C Communication completed"));
    //   Serial.println(); 

    //   wakeup_idle(TOTAL_IC); 
    //   error = LTC6811_rdcomm(TOTAL_IC,bms_ic); // Read comm register                 
    //   check_error(error);
    //   print_rxcomm(); // Print comm register   
    //   break;      

    case 29: // Clear all ADC measurement registers
      wakeup_sleep(TOTAL_IC);
      LTC6811_clrcell();
      LTC6811_clraux();
      LTC6811_clrstat();
      Serial.println(F("All Registers Cleared"));
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdcv(NO_OF_REG, TOTAL_IC,bms_ic); // read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);

      error = LTC6811_rdaux(NO_OF_REG,TOTAL_IC,bms_ic); // read back all aux registers
      check_error(error);
      print_aux(DATALOG_DISABLED);

      error = LTC6811_rdstat(NO_OF_REG,TOTAL_IC,bms_ic); // read back all stat 
      check_error(error);
      print_stat();           
      break;
        
    case 30: //Read CV,AUX and ADSTAT Voltages 
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      Serial.print(F("cell conversion completed in: "));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F(" ms"));
      Serial.println();
      wakeup_idle(TOTAL_IC);      
      error = LTC6811_rdcv(NO_OF_REG, TOTAL_IC,bms_ic); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);

      wakeup_sleep(TOTAL_IC);
      LTC6811_adax(ADC_CONVERSION_MODE , AUX_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      Serial.print(F("Aux conversion completed in: "));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F(" ms"));    
      Serial.println();
      wakeup_idle(TOTAL_IC); 
      error = LTC6811_rdaux(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all aux registers
      check_error(error);
      print_aux(DATALOG_DISABLED);   

      wakeup_sleep(TOTAL_IC);
      LTC6811_adstat(ADC_CONVERSION_MODE, STAT_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      Serial.print(F("Stat conversion completed in "));
      Serial.print(((float)conv_time/1000), 1);
      Serial.println(F(" ms"));
      Serial.println();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdstat(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all stat 
      check_error(error);
      print_stat();    
      break;

    case 31: // Set or reset the gpio pins(to drive output on gpio pins)
      /***********************************************************************
       Please ensure you have set the GPIO bits according to your requirement 
       in the configuration register.( check the global variable gpioBits_a )
      ************************************************************************/   
      wakeup_sleep(TOTAL_IC);
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        LTC6811_set_cfgr(current_ic,bms_ic,REFON,ADCOPT,gpioBits_a,dccBits_a, dctoBits, UV, OV);
      } 
      wakeup_idle(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,bms_ic);
      print_config();
      break;
	  
	  case 'm': //prints menu
      print_menu();
      break;

    default:
      Serial.println(F("Incorrect Option"));
      Serial.println();
      break;
  }
}

/*!*********************************
  \brief For Loop Measurement
 @return void
***********************************/
void measurement_loop(uint8_t datalog_en)
{
  int8_t error = 0;
  if (WRITE_CONFIG == ENABLED)
  {
    wakeup_sleep(TOTAL_IC);
    LTC6811_wrcfg(TOTAL_IC,bms_ic);
    print_config();
  }

  if (READ_CONFIG == ENABLED)
  {
    wakeup_sleep(TOTAL_IC);
    error = LTC6811_rdcfg(TOTAL_IC,bms_ic);
    check_error(error);
    print_rxconfig();
  }

  if (MEASURE_CELL == ENABLED)
  {
    wakeup_idle(TOTAL_IC);
    LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
    LTC6811_pollAdc();
    wakeup_idle(TOTAL_IC);
    error = LTC6811_rdcv(NO_OF_REG, TOTAL_IC,bms_ic);
    check_error(error);
    print_cells(datalog_en);

  }

  if (MEASURE_AUX == ENABLED)
  {
    wakeup_idle(TOTAL_IC);
    LTC6811_adax(ADC_CONVERSION_MODE , AUX_CH_ALL);
    LTC6811_pollAdc();
    wakeup_idle(TOTAL_IC);
    error = LTC6811_rdaux(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all aux registers
    check_error(error);
    print_aux(datalog_en);
  }

  if (MEASURE_STAT == ENABLED)
  {
    wakeup_idle(TOTAL_IC);
    LTC6811_adstat(ADC_CONVERSION_MODE, STAT_CH_ALL);
    LTC6811_pollAdc();
    wakeup_idle(TOTAL_IC);
    error = LTC6811_rdstat(NO_OF_REG,TOTAL_IC,bms_ic); // Set to read back all aux registers
    check_error(error);
    print_stat();
  }

  if (PRINT_PEC == ENABLED)
  {
    print_pec();
  }

}

/*!*********************************
  \brief Prints the main menu
 @return void
***********************************/
void print_menu()
{
  Serial.println(F("List of Commands: "));
  Serial.println(F("Write and Read Configuration: 1                             |Loop measurements with data-log output: 12                             |Write and Read of PWM: 23  "));
  Serial.println(F("Read Configuration: 2                                       |Run Mux Self Test: 13                                                  |Write and  Read of S control: 24 "));
  Serial.println(F("Start Cell Voltage Conversion: 3                            |Run ADC Self Test: 14                                                  |Clear S control register: 25 "));
  Serial.println(F("Read Cell Voltages: 4                                       |ADC overlap Test : 15                                                  |SPI Communication: 26 "));
  Serial.println(F("Start Aux Voltage Conversion: 5                             |Run Digital Redundancy Test: 16                                        |I2C Communication Write to Slave: 27"));
  Serial.println(F("Read Aux Voltages: 6                                        |Open Wire Test for single cell detection: 17                           |I2C Communication Read from Slave:28"));
  Serial.println(F("Start Stat Voltage Conversion: 7                            |Open Wire Test for multiple cell or two consecutive cells detection:18 |Clear Registers: 29"));
  Serial.println(F("Read Stat Voltages: 8                                       |Print PEC Counter: 19                                                  |Read CV,AUX and ADSTAT Voltages:30"));
  Serial.println(F("Start Combined Cell Voltage and GPIO1, GPIO2 Conversion: 9  |Reset PEC Counter: 20                                                  |Set or Reset the GPIO pins: 31 "));
  Serial.println(F("Start  Cell Voltage and Sum of cells : 10                   |Set Discharge: 21                                                      |"));
  Serial.println(F("loop Measurements: 11                                       |Clear Discharge: 22                                                    |"));
  Serial.println();
  Serial.println(F("Print 'm' for menu"));
  Serial.println(F("Please enter command: "));
  Serial.println();
}

/*!******************************************************************************
 \brief Prints the configuration data that is going to be written to the LTC6811
 to the serial port.
 @return void
 ********************************************************************************/
void print_config()
{
  int cfg_pec;

  Serial.println(F("Written Configuration: "));
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F("0x"));
    serial_print_hex(bms_ic[current_ic].config.tx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.tx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.tx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.tx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.tx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.tx_data[5]);
    Serial.print(F(", Calculated PEC: 0x"));
    cfg_pec = pec15_calc(6,&bms_ic[current_ic].config.tx_data[0]);
    serial_print_hex((uint8_t)(cfg_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println();
  }
  Serial.println();
}

/*!*****************************************************************
 \brief Prints the configuration data that was read back from the
 LTC6811 to the serial port.
  @return void
 *******************************************************************/
void print_rxconfig()
{
  Serial.println(F("Received Configuration: "));
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[5]);
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].config.rx_data[7]);
    Serial.println();
  }
  Serial.println();
}

/*!************************************************************
  \brief Prints cell voltage to the serial port
   @return void
 *************************************************************/
void print_cells(uint8_t datalog_en)
{
   for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.print(" IC ");
      Serial.print(current_ic+1,DEC);
      Serial.print(": ");
      for (int i=0; i<bms_ic[0].ic_reg.cell_channels; i++)
      {

        Serial.print(" C");
        Serial.print(i+1,DEC);
        Serial.print(":");
        Serial.print(bms_ic[current_ic].cells.c_codes[i]*0.0001,4);
        Serial.print(",");
      }
      Serial.println();
    }
    else
    {
      Serial.print("Cells ");
      
      for (int i=0; i<bms_ic[0].ic_reg.cell_channels; i++)
      {
        Serial.print(bms_ic[current_ic].cells.c_codes[i]*0.0001,4);
        Serial.print(",");
      }

    }
  }
  Serial.println();
}

/*!****************************************************************************
  \brief Prints GPIO voltage codes and Vref2 voltage code onto the serial port
 @return void
 *****************************************************************************/
void print_aux(uint8_t datalog_en)
{

  for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.print(" IC ");
      Serial.print(current_ic+1,DEC);
      Serial.print(":");
      
      for (int i=0; i < 5; i++)
      {
        Serial.print(F(" GPIO-"));
        Serial.print(i+1,DEC);
        Serial.print(":");
        Serial.print(bms_ic[current_ic].aux.a_codes[i]*0.0001,4);
        Serial.print(",");
      }
      Serial.print(F(" Vref2"));
      Serial.print(":");
      Serial.print(bms_ic[current_ic].aux.a_codes[5]*0.0001,4);
      Serial.println();
    }
    else
    {
      Serial.print("AUX ");

      for (int i=0; i < 6; i++)
      {
        Serial.print(bms_ic[current_ic].aux.a_codes[i]*0.0001,4);
        Serial.print(",");
      }
    }
  }
  Serial.println();
}

/*!****************************************************************************
  \brief Prints GPIO voltage codes (GPIO 1 & 2)
 @return void
 *****************************************************************************/
void print_aux1(uint8_t datalog_en)
{
  for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.print(" IC ");
      Serial.print(current_ic+1,DEC);
      Serial.print(F(": "));
      for (int i=0; i < 2; i++)
      {
        Serial.print(F(" GPIO-"));
        Serial.print(i+1,DEC);
        Serial.print(":");
        Serial.print(bms_ic[current_ic].aux.a_codes[i]*0.0001,4);
        Serial.print(",");
      }
    }
    else
    {
      Serial.print("AUX ");

      for (int i=0; i < 6; i++)
      {
        Serial.print(bms_ic[current_ic].aux.a_codes[i]*0.0001,4);
        Serial.print(",");
      }
    }
  }
  Serial.println();
}

/*!****************************************************************************
  \brief Prints Status voltage codes and Vref2 voltage code onto the serial port
 @return void
 *****************************************************************************/
void print_stat()
{
   double ITMP;
  for (uint8_t current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F(" SOC:"));
    Serial.print(bms_ic[current_ic].stat.stat_codes[0]*0.0001*20,4);
    Serial.print(F(","));
    Serial.print(F(" Itemp:"));
    ITMP = (double)((bms_ic[current_ic].stat.stat_codes[1] * (0.0001 / 0.0075)) - 273);   //Internal Die Temperature(°C) = ITMP • (100 µV / 7.5mV)°C - 273°C
    Serial.print(ITMP,4);
    Serial.print(F(","));
    Serial.print(F(" VregA:"));
    Serial.print(bms_ic[current_ic].stat.stat_codes[2]*0.0001,4);
    Serial.print(F(","));
    Serial.print(F(" VregD:"));
    Serial.print(bms_ic[current_ic].stat.stat_codes[3]*0.0001,4);
    Serial.println();
    Serial.print(F(" Flags:"));
    Serial.print(F(" 0x"));
    serial_print_hex(bms_ic[current_ic].stat.flags[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].stat.flags[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].stat.flags[2]);
    Serial.print(F("   Mux fail flag:"));
    Serial.print(F(" 0x"));
    serial_print_hex(bms_ic[current_ic].stat.mux_fail[0]);
    Serial.print(F("   THSD:"));
    Serial.print(F(" 0x"));
    serial_print_hex(bms_ic[current_ic].stat.thsd[0]);
    Serial.println();  
}
  Serial.println();
}

/*!****************************************************************************
  \brief Prints Status voltage codes for SOC onto the serial port
 @return void
 *****************************************************************************/
void print_statsoc()
{
 for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F(" SOC:"));
    Serial.print(bms_ic[current_ic].stat.stat_codes[0]*0.0001*20,4);
    Serial.print(F(","));
  }
  Serial.println();
}

/*****************************************************************************
  \brief Prints Open wire test results to the serial port
  @return void
 *****************************************************************************/
void print_open()
{
  for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (bms_ic[current_ic].system_open_wire == 65535)
    {
      Serial.print("No Opens Detected on IC ");
      Serial.print(current_ic+1, DEC);
      Serial.println();
    }
    else
    {
      Serial.print(F("There is an open wire on IC "));
      Serial.print(current_ic + 1,DEC);
      Serial.print(F(" Channel: "));
      Serial.println(bms_ic[current_ic].system_open_wire);
    }
  }
}

/*!******************************************************************************
 \brief Prints  PWM the configuration data that is going to be written to the LTC6811
 to the serial port.
  @return void
 ********************************************************************************/
void print_pwmconfig()
{
  int cfg_pec;

  Serial.println(F("Written PWM Configuration: "));
  for (uint8_t current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F("0x"));
    serial_print_hex(bms_ic[current_ic].pwm.tx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.tx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.tx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.tx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.tx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.tx_data[5]);
    Serial.print(F(", Calculated PEC: 0x"));
    cfg_pec = pec15_calc(6,&bms_ic[current_ic].pwm.tx_data[0]);
    serial_print_hex((uint8_t)(cfg_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println();
  }
  Serial.println();
}

/*!*****************************************************************
 \brief Prints the PWM configuration data that was read back from the
 LTC6811 to the serial port.
 @return void
 *******************************************************************/
void print_rxpwmconfig()
{
  Serial.println(F("Received PWM Configuration:"));
  for (uint8_t current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[5]);
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].pwm.rx_data[7]);
    Serial.println();
  }
  Serial.println();
}

/*!************************************************************
  \brief Prints comm register data to the serial port
  @return void
 *************************************************************/ 
void print_comm()
{
  uint8_t comm_pec;
  Serial.println(F("Written Data in COMM Register: "));
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F("0x"));
    serial_print_hex(bms_ic[current_ic].com.tx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.tx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.tx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.tx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.tx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.tx_data[5]);
    Serial.print(F(", Calculated PEC: 0x"));
    comm_pec = pec15_calc(6,&bms_ic[current_ic].com.tx_data[0]);
    serial_print_hex((uint8_t)(comm_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(comm_pec));
    Serial.println();
  }
  Serial.println();
}

/*!************************************************************
  \brief Prints comm register data that was read back from the
 LTC6811 to the serial port. 
 @return void
 *************************************************************/
void print_rxcomm()
{
  Serial.println(F("Received Data:"));
  for (uint8_t current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[5]);
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].com.rx_data[7]);
    Serial.println();
  }
  Serial.println();
}

/*!************************************************************
  \brief Prints S control register data to the serial port
  @return void
 *************************************************************/ 
void print_sctrl()
{
  int sctrl_pec;

  Serial.println(F("Written Data in sctrl register: "));
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F("0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.tx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.tx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.tx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.tx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.tx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.tx_data[5]);
    Serial.print(F(", Calculated PEC: 0x"));
    sctrl_pec = pec15_calc(6,&bms_ic[current_ic].sctrl.tx_data[0]);
    serial_print_hex((uint8_t)(sctrl_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(sctrl_pec));
    Serial.println();
  }
  Serial.println();
}

/*!************************************************************
  \brief Prints s control register data that was read back from the
 LTC6811 to the serial port.
@return void
 *************************************************************/
void print_rxsctrl()
{
  Serial.println(F("Received Data:"));
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[2]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[3]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[4]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[5]);
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(bms_ic[current_ic].sctrl.rx_data[7]);
    Serial.println();
  }
  Serial.println();
}

/*!************************************************************
  \brief Prints the PEC errors detected to the serial port
  @return void
 *************************************************************/ 
void print_pec()
{
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.println("");
    Serial.print(bms_ic[current_ic].crc_count.pec_count,DEC);
    Serial.print(F(" : PEC Errors Detected on IC"));
    Serial.println(current_ic+1,DEC);
  }
}

/*!************************************************************
  \brief Function to check error flag and print PEC error message
  @return void
 *************************************************************/ 
void check_error(int error)
{
  if (error == -1)
  {
    Serial.println(F("A PEC error was detected in the received data"));
  }
}

/*!************************************************************
 \brief Function to print in HEX form
 @return void
 *************************************************************/ 
void serial_print_hex(uint8_t data)
{
  if (data< 16)
  {
    Serial.print("0");
    Serial.print((byte)data,HEX);
  }
  else
    Serial.print((byte)data,HEX);
}

/*!************************************************************
 \brief Hex conversion constants
 *************************************************************/
char hex_digits[16]=
{
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*!************************************************************
 \brief Global Variables
 *************************************************************/
char hex_to_byte_buffer[5]=
{
  '0', 'x', '0', '0', '\0'
};               

/*!************************************************************
 \brief Buffer for ASCII hex to byte conversion
 *************************************************************/
char byte_to_hex_buffer[3]=
{
  '\0','\0','\0'
};

/*!************************************************************
 \brief Read 2 hex characters from the serial buffer and convert them to a byte
 @return char data Read Data
 *************************************************************/
char read_hex()    
{
  byte data;
  hex_to_byte_buffer[2]=get_char();
  hex_to_byte_buffer[3]=get_char();
  get_char();
  get_char();
  data = strtol(hex_to_byte_buffer, NULL, 0);
  return(data);
}

/*!************************************************************
 \brief Read a command from the serial port
 @return char 
 *************************************************************/
char get_char()
{

  while (Serial.available() <= 0);
  return(Serial.read());
}
