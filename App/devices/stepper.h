/**
  * @file    stepper.h
  * @brief   3 dong co buoc qua driver PUL/DIR/ENA/ALM tren TIM1.
  *          Moi kenh tan so rieng (Output Compare Toggle), ramp hinh thang.
  */
#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>
#include "board_config.h"
#include "test_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
#define X(name, ch, pp, pn, dp, dn, ep, en, ap, an, inv) STEP_##name,
  STEPPER_LIST(X)
#undef X
  STEP_COUNT
} step_id_t;

extern const test_module_t stepper_module;

/* ---- API cho module khac / scenario ---- */
int     Step_Move(step_id_t id, int32_t steps, uint32_t sps, uint32_t acc); /* 0 = OK */
int     Step_Run(step_id_t id, int32_t sps, uint32_t acc);  /* chay lien tuc, dau = chieu */
void    Step_Stop(step_id_t id);       /* giam toc roi dung */
void    Step_Halt(step_id_t id);       /* dung ngay, khong ramp */
void    Step_Enable(step_id_t id, uint8_t on);
uint8_t Step_IsBusy(step_id_t id);
int32_t Step_GetPos(step_id_t id);
uint8_t Step_IsAlarm(step_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* STEPPER_H */
