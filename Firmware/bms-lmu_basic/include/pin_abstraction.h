#include <Arduino.h>

class DigitalOut {
  private:
    uint32_t _pin_number;

  public:
    DigitalOut(uint32_t pin_number){
      _pin_number = pin_number;
      pinMode(_pin_number, OUTPUT);
    }

    int read(){
      return digitalRead(_pin_number);
    }

    void write(uint32_t state){
      digitalWrite(_pin_number, state);
    }

    DigitalOut &operator= (uint32_t value){
      digitalWrite(_pin_number, value);
      return *this;
    }

    DigitalOut &operator= (DigitalOut &rhs);

    operator int() {
      return read();
    }
};

class DigitalIn {
    private:
        uint32_t _pin_number;

    public:
        DigitalIn(uint32_t pin_number){
            _pin_number = pin_number;
            pinMode(_pin_number, INPUT);
        }

        int read() {
            return digitalRead(_pin_number);
        }

        operator int(){
            return read();
        }
};