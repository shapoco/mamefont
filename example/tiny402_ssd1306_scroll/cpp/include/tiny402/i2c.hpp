#pragma once

#ifndef __AVR_ATtiny402__
#define __AVR_ATtiny402__
#endif

#ifndef F_CPU
#define F_CPU 20000000UL
#endif

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

// MCTRLA
//   [7]    RIEN: Read Interrupt Enable
//   [6]    WIEN: Write Interrupt Enable
//   [4]    QCEN: Quick Command Enable
//   [3:2]  TIMEOUT: 0=DISABLED, 1=50us, 2=100us, 3=200us
//   [1]    SMEN: Smart Mode Enable
//   [0]    ENABLE

// MCTRLB
//   [3]    FLUSH
//   [2]    ACKACT
//   [1:0]  MCMD: 0=NOACT, 1=REPSTART, 2=RECVTRANS, 3=STOP

enum class I2cPin : uint8_t {
  SDA = PIN1_bm,
  SCL = PIN2_bm,
};

class I2C {
 public:
  static constexpr uint8_t getBaud(uint32_t cpuFreq, uint32_t sclFreq,
                                   uint32_t tRise) {
    return (((float)cpuFreq / sclFreq) - 10 -
            ((float)cpuFreq * tRise / 1000000)) /
           2;
    // uint32_t tRiseNs = (sclFreq <= 100000ul)   ? 500
    //                    : (sclFreq <= 400000ul) ? 200
    //                                            : 100;
    // return cpuFreq / (2 * sclFreq) - (5 + cpuFreq * tRiseNs / 2);
  }

  void beginWithBaud(uint8_t baud) {
    TWI0.MBAUD = baud;
    TWI0.MCTRLA = TWI_ENABLE_bm | TWI_TIMEOUT_DISABLED_gc;
    TWI0.MCTRLB |= TWI_FLUSH_bm;
    TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;
    TWI0.MSTATUS |= TWI_RIF_bm | TWI_WIF_bm;
  }

  void start(uint8_t devAddrRxW) {
    TWI0.MADDR = devAddrRxW;
    TWI0.MCTRLB |= TWI_MCMD_REPSTART_gc;
  }

  void write(const uint8_t data) {
    waitIdle();
    TWI0.MDATA = data;
    TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;
  }

  void end() {
    waitIdle();
    TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
  }

  void waitIdle() { while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm))); }

  void disable() { TWI0.MCTRLA &= ~TWI_ENABLE_bm; }

  void enable() {
    PORTA_DIRCLR = (uint8_t)I2cPin::SDA | (uint8_t)I2cPin::SCL;
    TWI0.MCTRLA |= TWI_ENABLE_bm;
    TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;
  }

  void put(I2cPin pin, bool value) {
    if (value) {
      PORTA_OUTSET = (uint8_t)pin;
      PORTA_DIRSET = (uint8_t)pin;
    } else {
      PORTA_DIRCLR = (uint8_t)pin;
    }
  }

};  // namespace i2c
