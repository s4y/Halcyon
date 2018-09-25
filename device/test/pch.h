#pragma once 

#include <inttypes.h>

#define __CORE_CM4_H_GENERIC
#define __CMSIS_GENERIC

#define __I volatile const
#define __O volatile
#define __IO volatile
#define __IM volatile const
#define __OM volatile
#define __IOM volatile

#define __STATIC_INLINE static inline
#define SVCALL(number, return_type, signature)

#include "nrf52.h"
#include "nrf52_bitfields.h"
#include "nrf52832_peripherals.h"
#include "nrf51_to_nrf52.h"

#undef NRF_FICR_BASE
#undef NRF_UICR_BASE
#undef NRF_BPROT_BASE
#undef NRF_POWER_BASE
#undef NRF_CLOCK_BASE
#undef NRF_RADIO_BASE
#undef NRF_UARTE0_BASE
#undef NRF_UART0_BASE
#undef NRF_SPIM0_BASE
#undef NRF_SPIS0_BASE
#undef NRF_TWIM0_BASE
#undef NRF_TWIS0_BASE
#undef NRF_SPI0_BASE
#undef NRF_TWI0_BASE
#undef NRF_SPIM1_BASE
#undef NRF_SPIS1_BASE
#undef NRF_TWIM1_BASE
#undef NRF_TWIS1_BASE
#undef NRF_SPI1_BASE
#undef NRF_TWI1_BASE
#undef NRF_NFCT_BASE
#undef NRF_GPIOTE_BASE
#undef NRF_SAADC_BASE
#undef NRF_TIMER0_BASE
#undef NRF_TIMER1_BASE
#undef NRF_TIMER2_BASE
#undef NRF_RTC0_BASE
#undef NRF_TEMP_BASE
#undef NRF_RNG_BASE
#undef NRF_ECB_BASE
#undef NRF_CCM_BASE
#undef NRF_AAR_BASE
#undef NRF_WDT_BASE
#undef NRF_RTC1_BASE
#undef NRF_QDEC_BASE
#undef NRF_COMP_BASE
#undef NRF_LPCOMP_BASE
#undef NRF_SWI0_BASE
#undef NRF_EGU0_BASE
#undef NRF_SWI1_BASE
#undef NRF_EGU1_BASE
#undef NRF_SWI2_BASE
#undef NRF_EGU2_BASE
#undef NRF_SWI3_BASE
#undef NRF_EGU3_BASE
#undef NRF_SWI4_BASE
#undef NRF_EGU4_BASE
#undef NRF_SWI5_BASE
#undef NRF_EGU5_BASE
#undef NRF_TIMER3_BASE
#undef NRF_TIMER4_BASE
#undef NRF_PWM0_BASE
#undef NRF_PDM_BASE
#undef NRF_NVMC_BASE
#undef NRF_PPI_BASE
#undef NRF_MWU_BASE
#undef NRF_PWM1_BASE
#undef NRF_PWM2_BASE
#undef NRF_SPIM2_BASE
#undef NRF_SPIS2_BASE
#undef NRF_SPI2_BASE
#undef NRF_RTC2_BASE
#undef NRF_I2S_BASE
#undef NRF_FPU_BASE
#undef NRF_P0_BASE

typedef struct {
  __IOM uint32_t ISER[8U];               /*!< Offset: 0x000 (R/W)  Interrupt Set Enable Register */
        uint32_t RESERVED0[24U];
  __IOM uint32_t ICER[8U];               /*!< Offset: 0x080 (R/W)  Interrupt Clear Enable Register */
        uint32_t RSERVED1[24U];
  __IOM uint32_t ISPR[8U];               /*!< Offset: 0x100 (R/W)  Interrupt Set Pending Register */
        uint32_t RESERVED2[24U];
  __IOM uint32_t ICPR[8U];               /*!< Offset: 0x180 (R/W)  Interrupt Clear Pending Register */
        uint32_t RESERVED3[24U];
  __IOM uint32_t IABR[8U];               /*!< Offset: 0x200 (R/W)  Interrupt Active bit Register */
        uint32_t RESERVED4[56U];
  __IOM uint8_t  IP[240U];               /*!< Offset: 0x300 (R/W)  Interrupt Priority Register (8Bit wide) */
        uint32_t RESERVED5[644U];
  __OM  uint32_t STIR;                   /*!< Offset: 0xE00 ( /W)  Software Trigger Interrupt Register */
}  NVIC_Type;

extern NRF_GPIO_Type* NRF_P0_BASE;
extern NRF_UARTE_Type* NRF_UARTE0_BASE;
extern NVIC_Type* NVIC;

static inline void NVIC_ClearPendingIRQ(IRQn_Type IRQn) {}
static inline void NVIC_EnableIRQ(IRQn_Type IRQn) {}
static inline void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority) {}

extern void UARTE0_UART0_IRQHandler();
