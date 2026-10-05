/**
  * @file    cylinder.h
  * @brief   Xi lanh dien qua driver PWM + DIR. API khong blocking + lenh console.
  */
#ifndef CYLINDER_H
#define CYLINDER_H

#include <stdint.h>
#include "board_config.h"
#include "test_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
#define X(name, tim, ch, dport, dpin) CYL_##name,
  CYL_LIST(X)
#undef X
  CYL_COUNT
} cyl_id_t;

extern const test_module_t cylinder_module;

/* ---- API cho module khac / scenario ---- */
void    Cyl_Extend(cyl_id_t id, uint8_t duty, uint32_t ms);  /* day ra */
void    Cyl_Retract(cyl_id_t id, uint8_t duty, uint32_t ms); /* rut ve */
void    Cyl_Stop(cyl_id_t id);                               /* dung ngay */
uint8_t Cyl_IsBusy(cyl_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* CYLINDER_H */
