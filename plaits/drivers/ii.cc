#include "plaits/drivers/ii.h"

#include <stm32f37x_conf.h>

#define II_I2C_ADDRESS                 0x49
#define II_I2C                         I2C1
#define II_I2C_CLK                     RCC_APB1Periph_I2C1
#define II_I2C_GPIO_CLOCK              RCC_AHBPeriph_GPIOB
#define II_I2C_GPIO_AF                 GPIO_AF_4
#define II_I2C_GPIO                    GPIOB
#define II_I2C_SCL_PIN                 GPIO_Pin_8
#define II_I2C_SDA_PIN                 GPIO_Pin_9
#define II_I2C_SCL_PINSRC              GPIO_PinSource8
#define II_I2C_SDA_PINSRC              GPIO_PinSource9
#define II_TIMEOUT                     ((uint32_t)0x1000)
#define II_LONG_TIMEOUT                ((uint32_t)(300 * II_TIMEOUT))
// beehive GPIOA SCL=15 SDA=14
// retail GPIOB SCL=8 SDA=9

namespace plaits {

void II::InitializeGPIO() {
  // Start GPIO peripheral clocks.
  RCC_AHBPeriphClockCmd(II_I2C_GPIO_CLOCK, ENABLE);

  // Initialize I2C pins
  GPIO_InitTypeDef gpio_init;
  gpio_init.GPIO_Pin = II_I2C_SCL_PIN | II_I2C_SDA_PIN; 
  gpio_init.GPIO_Mode = GPIO_Mode_AF;
  gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
  gpio_init.GPIO_OType = GPIO_OType_OD;
  gpio_init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(II_I2C_GPIO, &gpio_init);

  // Connect pins to I2C peripheral
  GPIO_PinAFConfig(II_I2C_GPIO, II_I2C_SCL_PINSRC, II_I2C_GPIO_AF);
  GPIO_PinAFConfig(II_I2C_GPIO, II_I2C_SDA_PINSRC, II_I2C_GPIO_AF);
}

void II::InitializeControlInterface() {
  I2C_InitTypeDef i2c_init;

  // Initialize I2C
  RCC_I2CCLKConfig(RCC_I2C1CLK_SYSCLK);
  RCC_APB1PeriphClockCmd(II_I2C_CLK, ENABLE);

  I2C_DeInit(II_I2C);
  i2c_init.I2C_Mode = I2C_Mode_I2C;
  i2c_init.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
  i2c_init.I2C_DigitalFilter = 0x00;
  i2c_init.I2C_OwnAddress1 = II_I2C_ADDRESS << 1;
  i2c_init.I2C_Ack = I2C_Ack_Enable;
  i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  i2c_init.I2C_Timing = 0xb0420f13; // 100 KHz

  I2C_Init(II_I2C, &i2c_init);

  NVIC_SetPriority(I2C1_EV_IRQn, 2);
	NVIC_EnableIRQ(I2C1_EV_IRQn);

  I2C_Cmd(II_I2C, ENABLE);
  I2C_ITConfig(II_I2C, I2C_IT_ADDRI, ENABLE);
}

void II::Init(Modulations* modulations) {
  modulations_ = modulations;

  next_buffer_byte_ = 0;

  InitializeGPIO();
  InitializeControlInterface();
}

bool II::IRQHandler() {
  bool returnval = false;
  if (I2C_GetITStatus(II_I2C, I2C_IT_ADDR) == SET) {
    I2C_ITConfig(II_I2C, I2C_IT_RXI | I2C_IT_TXI | I2C_IT_STOPI, ENABLE);
    ProcessBuffer();
    returnval = true;
    I2C_ClearITPendingBit(II_I2C, I2C_IT_ADDR);
  }
  if (I2C_GetITStatus(II_I2C, I2C_IT_RXNE) == SET) {
    if(next_buffer_byte_ < I2C_BUFFER_BYTES) {
      buffer_[next_buffer_byte_] = I2C_ReceiveData(II_I2C);
      next_buffer_byte_++;
    }
	}
  if (I2C_GetITStatus(II_I2C, I2C_IT_STOPF) == SET) {
    ProcessBuffer();
    returnval = true;

		I2C_ClearITPendingBit(II_I2C, I2C_IT_STOPF);
    I2C_ITConfig(II_I2C, I2C_IT_RXI | I2C_IT_TXI | I2C_IT_STOPI, DISABLE);

		I2C_Cmd(II_I2C, DISABLE);
		I2C_ClearITPendingBit(II_I2C, I2C_IT_TXIS);
		I2C_Cmd(II_I2C, ENABLE);

		I2C_ITConfig(II_I2C, I2C_IT_ADDRI, ENABLE);
  }

  return returnval;
}

void II::ProcessBuffer() {
  if(next_buffer_byte_ == 0) {
    return;
  }
  
  // look for commands with required byte length
  if(buffer_[0] == II_CTL && next_buffer_byte_ == 3) {
    uint8_t ctrl = buffer_[1];
    bool value = buffer_[2] == 0x01;
    if(ctrl == II_CTL_ALL) {
      for(int i = II_CTL_ALL+1; i < II_CTL_LAST; i++) {
        controls_[i].enabled = value;
      }
    } else if(ctrl > II_CTL_ALL && ctrl < II_CTL_LAST) {
      controls_[ctrl].enabled = value;
    }
  }
  else if(buffer_[0] == II_TRIGGER) {
    controls_[II_CTL_TRIGGER].received = true;
  }
  else if(buffer_[0] == II_LEVEL && next_buffer_byte_ == 3) {
    uint16_t val = (buffer_[1] << 8) + buffer_[2];
    controls_[II_CTL_LEVEL].value = static_cast<float>(val) / 16384.0f;
  }  
  else if(buffer_[0] == II_VOCT && next_buffer_byte_ == 3) {
    uint16_t val = (buffer_[1] << 8) + buffer_[2];
    controls_[II_CTL_VOCT].value = static_cast<float>(val) * (120.0f / 16384.0f);
  }

  next_buffer_byte_ = 0;
}

void II::Poll() {
  if(controls_[II_CTL_TRIGGER].enabled) {
    modulations_->trigger_patched = true;
    if(controls_[II_CTL_TRIGGER].received) {
      controls_[II_CTL_TRIGGER].received = false;
      modulations_->trigger = 0.5;
    } else {
      modulations_->trigger = 0.0;
    }
  }

  if(controls_[II_CTL_LEVEL].enabled) {
    modulations_->level_patched = true;
    modulations_->level = controls_[II_CTL_LEVEL].value;
  }

  if(controls_[II_CTL_VOCT].enabled) {
    modulations_->note = controls_[II_CTL_VOCT].value;
  }
}

}  // namespace plaits
