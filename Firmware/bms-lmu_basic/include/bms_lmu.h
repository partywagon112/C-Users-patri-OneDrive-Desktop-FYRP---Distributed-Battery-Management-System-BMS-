#include <Arduino.h>
#include "pin_abstraction.h"

// State Machine Logic
enum LMU_States {
  FAULT, 
  IDLE, 
  MEASURING,
  PASSIVE_BALANCING, 
  ACTIVE_BALANCING
} lmu_state_t;

typedef enum ERROR_CODES_SUB_KEY{
    ERROR_OV_FAULT,
    ERROR_UV_FAULT,
    ERROR_OT_FAULT,
    ERROR_RELAY_FAULT
} error_state_t;


class Heartbeat {
  private:
    DigitalOut fault_relay;
    DigitalOut heartbeat_led;

    DigitalIn fault_relay_feedback;

    int _counter;
    LMU_States _state;
    uint8_t _fault_code;

  public:
    Heartbeat(uint32_t fault_relay_pin, uint32_t fault_relay_feedback_pin, uint32_t heartbeat_led_pin) : 
    fault_relay(fault_relay_pin), fault_relay_feedback(fault_relay_feedback_pin), heartbeat_led(heartbeat_led_pin) {
      _counter = 0;
      _state = FAULT;
    }

    // increments counter if healthy. must be associated with a timer of some sort.
    void tick(uint8_t fault_code){
        _fault_code = fault_code;
      if (fault_code == 0){
        _counter = _counter + 1;
        fault_relay = 1;
        heartbeat_led = !heartbeat_led;
      } else {
        _counter = 0;
        fault_relay = 0;
      }
    }

    int counter(){
      return _counter;
    }

    LMU_States state(){
      return _state;
    }

    void state(LMU_States new_state){
      _state = new_state;
    }

    bool relay_fault(){
        return fault_relay != fault_relay_feedback;
    }

    uint8_t fault_code(){
        return _fault_code;
    }

    void fault_code(uint16_t fault_code){
        _fault_code = fault_code;
    }
};

