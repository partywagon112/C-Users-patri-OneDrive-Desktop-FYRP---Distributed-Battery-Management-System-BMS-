#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "SPI.h"
#include "LT_SPI.h"
#include "LTC6811.h"
#include "LTC681x.h"

#define STACK_SIZE 12

// SAFETY PARAMETERS!
#define MAX_VOLTAGE 4.2
#define MIN_VOLTAGE 2.8
#define MAX_TEMPERATURE 60


class Stack {
  private: 
    uint16_t cells[STACK_SIZE];

  public:
    bool update_cell(int pos, uint16_t cell_voltage){
      if (pos >= STACK_SIZE || pos < 0) {
        return false;
      } else {
        cells[pos] = cell_voltage;
        return true;
      }
    }

    uint16_t min(){
      uint16_t lowest_cell = cells[0];
      for (int i = 0; i < STACK_SIZE; i++){
        if (lowest_cell > cells[i]) {
          lowest_cell = cells[i];
        }
      }
      return lowest_cell;
    }

    uint16_t max(){
      uint16_t highest_cell = cells[0];
      for (int i = 0; i < STACK_SIZE; i++){
        if (highest_cell < cells[i]) {
          highest_cell = cells[i];
        }
      }
      return highest_cell;
    }

    float average(){
      return (float)sum_stack_voltage()/STACK_SIZE;
    }

    int sum_stack_voltage(){
      int sum = 0;
      for (int i = 0; i < STACK_SIZE; i++){
        sum = sum + cells[i];
      }
      return sum;
    }

    bool ov_fault(){
        return (max() > MAX_VOLTAGE);
    }

    bool uv_fault(){
        return (min() < MIN_VOLTAGE);
    }

    uint16_t cell_voltage(int cell_number){
        return cells[cell_number];
    }
};


// class PBalancer {
//   // Ethan's stuff go here
//   private:
//     Stack _stack;
//     uint16_t _discharge_time_limit;

//     TickerInterrupt measure_ticker;
    

//     const uint8_t STREG=0;
//     int8_t error = 0;
//     uint32_t conv_time = 0;
//     uint32_t user_command;
//     int8_t readIC=0;
//     char input = 0;
    
//   public:
//     PBalancer(Stack &stack, uint16_t dtl): measure_ticker(TIM3, 1){
//       _stack = stack;
//       _discharge_time_limit = dtl;

//     }

//     void setup(){
//       // quikeval_SPI_connect();
//       spi_enable(SPI_CLOCK_DIV16); // This will set the Linduino to have a 1MHz Clock
//       LTC6811_init_cfg(TOTAL_IC, bms_ic);
//       for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
//       {
//         LTC6811_set_cfgr(current_ic,bms_ic,REFON,ADCOPT,gpioBits_a,dccBits_a, dctoBits, UV, OV);
//       }
//       LTC6811_reset_crc_count(TOTAL_IC,bms_ic);
//       LTC6811_init_reg_limits(TOTAL_IC,bms_ic);
      
//       print_config();
//     }

//     void read_config(){
//       wakeup_sleep(TOTAL_IC);
//       LTC6811_wrcfg(TOTAL_IC,bms_ic);
//       print_config();
//       wakeup_idle(TOTAL_IC);
//       error = LTC6811_rdcfg(TOTAL_IC,bms_ic);
//       check_error(error);
//       print_rxconfig();
//     }

//     void measure_stack(){
//       wakeup_sleep(TOTAL_IC);
//       LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
//       conv_time = LTC6811_pollAdc();
//       Serial.print(F("Cell conversion completed in:"));
//       Serial.print(((float)conv_time/1000), 1);
//       Serial.println(F("ms"));
//       Serial.println();
//     }

//     void read_cell_voltage_registers(){// Read Cell Voltage Registers
//       wakeup_sleep(TOTAL_IC);
//       error = LTC6811_rdcv(NO_OF_REG, TOTAL_IC,bms_ic); // Set to read back all cell voltage registers
//       check_error(error);
//       print_cells(DATALOG_DISABLED);
//     }

//     uint8_t get_errors(){
//       return error;
//     }

//     void start_balance(){

//     }

//     void end_balance(){

//     }

//     void update_stack(){

//     }

//     bool pbalance_ok(){

//     }
// };