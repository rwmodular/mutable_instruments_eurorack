#ifndef PLAITS_DRIVERS_II_H_
#define PLAITS_DRIVERS_II_H_

#include "stmlib/stmlib.h"
#include "stm32f37x.h"
#include "plaits/dsp/voice.h"

#define I2C_BUFFER_BYTES 4

namespace plaits {

// Commands
enum Cmds {
  II_NA,
  II_CTL,       // Exclusive control <TARGET><1/0>
  II_TRIGGER,   // Trigger (none)
  II_LEVEL,     // Level  <MSB><LSB> (0 - 16384)
  II_VOCT       // V/Oct  <MSB><LSB> (0 - 16384, 1638.4 / oct)
};

// Control Targets
enum Ctrls {
  II_CTL_ALL,
  II_CTL_TRIGGER,
  II_CTL_LEVEL,
  II_CTL_VOCT,
  II_CTL_LAST
};

struct IIControl {
  bool enabled;
  bool received;
  float value;
};

class II {
 public:
  II() { }
  ~II() { }
  
  void Init(Modulations* modulations);
  bool IRQHandler();
  void ProcessBuffer();
  void Poll();

 private:
  void InitializeGPIO();
  void InitializeControlInterface();

  Modulations* modulations_;

  uint8_t next_buffer_byte_;
  uint8_t buffer_[I2C_BUFFER_BYTES];

  IIControl controls_[II_CTL_LAST];

  DISALLOW_COPY_AND_ASSIGN(II);
};

}  // namespace plaits

#endif  // PLAITS_DRIVERS_II_H_
