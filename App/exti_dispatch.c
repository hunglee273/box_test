/**
  * @file    exti_dispatch.c
  * @brief   HAL_GPIO_EXTI_Callback duy nhat cua project, chuyen ngat EXTI
  *          den module tuong ung. Module moi dung EXTI thi them vao day.
  */
#include "board_config.h"

#if TEST_EN_OPTICAL
#include "optical.h"
#endif

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if TEST_EN_OPTICAL
  Optical_OnExti(GPIO_Pin);
#endif
  (void)GPIO_Pin;
}
