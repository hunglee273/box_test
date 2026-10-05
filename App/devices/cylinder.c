/**
  * @file    cylinder.c
  * @brief   Xi lanh dien qua driver PWM + DIR (333 Hz).
  *          Moi lenh chay co thoi gian gioi han; doi chieu luon dung han
  *          CYL_DEADTIME_MS truoc; duty tang dan trong CYL_RAMP_MS.
  */
#include "cylinder.h"
#include "console.h"
#include <string.h>
#include <strings.h>

typedef struct
{
  const char        *name;
  TIM_HandleTypeDef *tim;
  uint32_t           ch;
  GPIO_TypeDef      *dir_port;
  uint16_t           dir_pin;
} cyl_cfg_t;

typedef enum { CS_IDLE, CS_DEAD, CS_RUN } cyl_state_t;

typedef struct
{
  cyl_state_t state;
  int8_t      dir;        /* +1 day ra, -1 rut ve, 0 chua chay lan nao */
  int8_t      next_dir;
  uint8_t     duty;       /* duty dang xuat, % */
  uint8_t     target;     /* duty dich, % */
  uint32_t    run_ms;
  uint32_t    t_start;
  uint32_t    deadline;
} cyl_state_s;

static const cyl_cfg_t cfg[CYL_COUNT] =
{
#define X(name, tim, ch, dport, dpin) { #name, tim, ch, dport, dpin },
  CYL_LIST(X)
#undef X
};

static cyl_state_s st[CYL_COUNT];

/* ---------------------------------------------------------------------- */

static uint8_t expired(uint32_t now, uint32_t t)
{
  return (int32_t)(now - t) >= 0;
}

static void set_duty(int i, uint8_t duty)
{
  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(cfg[i].tim);
  st[i].duty = duty;
  __HAL_TIM_SET_COMPARE(cfg[i].tim, cfg[i].ch, (arr + 1U) * duty / 100U);
}

static void set_dir(int i, int8_t dir)
{
  GPIO_PinState ext = CYL_DIR_EXTEND_LEVEL;
  GPIO_PinState ret = (ext == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
  HAL_GPIO_WritePin(cfg[i].dir_port, cfg[i].dir_pin, dir > 0 ? ext : ret);
  st[i].dir = dir;
}

static const char *dir_str(int8_t dir)
{
  return dir > 0 ? "day ra" : (dir < 0 ? "rut ve" : "-");
}

static void begin_run(int i, uint32_t now)
{
  set_dir(i, st[i].next_dir);
  st[i].t_start = now;
  st[i].deadline = now + st[i].run_ms;
  st[i].state = CS_RUN;
  printf("[cyl] %s %s duty %u%% trong %lu ms\n", cfg[i].name,
         dir_str(st[i].dir), st[i].target, (unsigned long)st[i].run_ms);
}

static void request(int i, int8_t dir, uint8_t duty, uint32_t ms)
{
  uint32_t now = HAL_GetTick();

  if (duty > CYL_DUTY_MAX) duty = CYL_DUTY_MAX;
  if (ms > CYL_T_MAX_MS) ms = CYL_T_MAX_MS;

  st[i].next_dir = dir;
  st[i].target = duty;
  st[i].run_ms = ms;

  if (st[i].dir == dir && st[i].state == CS_RUN)
  {
    /* Cung chieu dang chay: chi cap nhat duty/thoi gian */
    st[i].deadline = now + ms;
    printf("[cyl] %s cap nhat: duty %u%%, con %lu ms\n", cfg[i].name, duty, (unsigned long)ms);
    return;
  }
  if (st[i].dir == dir && st[i].duty == 0)
  {
    begin_run(i, now);
    return;
  }

  /* Doi chieu (hoac lan dau): dung han roi moi dao DIR */
  set_duty(i, 0);
  st[i].deadline = now + CYL_DEADTIME_MS;
  st[i].state = CS_DEAD;
}

/* ---- API --------------------------------------------------------------- */

void Cyl_Extend(cyl_id_t id, uint8_t duty, uint32_t ms)
{
  if (id < CYL_COUNT) request(id, +1, duty, ms);
}

void Cyl_Retract(cyl_id_t id, uint8_t duty, uint32_t ms)
{
  if (id < CYL_COUNT) request(id, -1, duty, ms);
}

void Cyl_Stop(cyl_id_t id)
{
  if (id >= CYL_COUNT) return;
  set_duty(id, 0);
  st[id].state = CS_IDLE;
}

uint8_t Cyl_IsBusy(cyl_id_t id)
{
  return (id < CYL_COUNT) && (st[id].state != CS_IDLE);
}

/* ---- Module ------------------------------------------------------------ */

static void cyl_init(void)
{
  for (int i = 0; i < CYL_COUNT; i++)
  {
    set_duty(i, 0);
    /* PWM chay lien tuc, duty 0 = chan luon LOW. Khong tao UG vi TIM4
       dung chung voi servo. */
    HAL_TIM_PWM_Start(cfg[i].tim, cfg[i].ch);
    st[i].state = CS_IDLE;
    st[i].dir = 0;
  }
}

static void cyl_safe(void)
{
  for (int i = 0; i < CYL_COUNT; i++) Cyl_Stop((cyl_id_t)i);
}

static void cyl_poll(uint32_t now)
{
  for (int i = 0; i < CYL_COUNT; i++)
  {
    cyl_state_s *s = &st[i];

    switch (s->state)
    {
      case CS_DEAD:
        if (expired(now, s->deadline)) begin_run(i, now);
        break;

      case CS_RUN:
        if (expired(now, s->deadline))
        {
          set_duty(i, 0);
          s->state = CS_IDLE;
          printf("[cyl] %s dung (het %lu ms)\n", cfg[i].name, (unsigned long)s->run_ms);
        }
        else if (s->duty != s->target)
        {
          uint32_t el = now - s->t_start;
          uint32_t d = (CYL_RAMP_MS == 0 || el >= CYL_RAMP_MS)
                         ? s->target : s->target * el / CYL_RAMP_MS;
          if (d != s->duty) set_duty(i, (uint8_t)d);
        }
        break;

      case CS_IDLE:
      default:
        break;
    }
  }
}

static void cyl_help(void)
{
  printf("cyl                          trang thai\n"
         "cyl <L|R|all> ext [duty] [ms] day ra   (mac dinh %u%%, %u ms)\n"
         "cyl <L|R|all> ret [duty] [ms] rut ve\n"
         "cyl <L|R|all> stop           dung ngay\n"
         "duty 0..%u %%, ms toi da %u. Doi chieu tu dung %u ms truoc.\n"
         "Sai chieu: doi CYL_DIR_EXTEND_LEVEL trong board_config.h\n",
         CYL_DUTY_DEFAULT, CYL_T_DEFAULT_MS, CYL_DUTY_MAX, CYL_T_MAX_MS, CYL_DEADTIME_MS);
}

static void print_status(void)
{
  static const char *const names[] = { "idle", "doi chieu", "chay" };
  uint32_t now = HAL_GetTick();
  for (int i = 0; i < CYL_COUNT; i++)
  {
    printf("[cyl] %s: %-9s chieu %-6s duty %3u%% DIR=%s",
           cfg[i].name, names[st[i].state], dir_str(st[i].dir), st[i].duty,
           HAL_GPIO_ReadPin(cfg[i].dir_port, cfg[i].dir_pin) == GPIO_PIN_SET ? "HIGH" : "LOW");
    if (st[i].state == CS_RUN)
    {
      printf(" con %lu ms", (unsigned long)(st[i].deadline - now));
    }
    printf("\n");
  }
}

static void cyl_cmd(int argc, char *argv[])
{
  if (argc < 3)
  {
    if (argc == 2 && strcmp(argv[1], "help") == 0) cyl_help();
    else print_status();
    return;
  }

  int first = -1, last = -1;
  if (strcasecmp(argv[1], "all") == 0)
  {
    first = 0;
    last = CYL_COUNT - 1;
  }
  else
  {
    for (int i = 0; i < CYL_COUNT; i++)
    {
      if (strcasecmp(argv[1], cfg[i].name) == 0) first = last = i;
    }
  }
  if (first < 0)
  {
    printf("ERR: khong co xi lanh '%s'\n", argv[1]);
    return;
  }

  const char *sub = argv[2];
  if (strcmp(sub, "stop") == 0)
  {
    for (int i = first; i <= last; i++) Cyl_Stop((cyl_id_t)i);
    print_status();
    return;
  }

  int8_t dir;
  if (strcmp(sub, "ext") == 0) dir = +1;
  else if (strcmp(sub, "ret") == 0) dir = -1;
  else
  {
    cyl_help();
    return;
  }

  int32_t duty = CYL_DUTY_DEFAULT, ms = CYL_T_DEFAULT_MS;
  if ((argc >= 4 && !Console_ParseInt(argv[3], &duty)) ||
      (argc >= 5 && !Console_ParseInt(argv[4], &ms)))
  {
    printf("ERR: cyl %s %s [duty] [ms]\n", argv[1], sub);
    return;
  }
  if (duty < 0 || duty > CYL_DUTY_MAX)
  {
    printf("ERR: duty 0..%u\n", CYL_DUTY_MAX);
    return;
  }
  if (ms <= 0 || ms > CYL_T_MAX_MS)
  {
    printf("ERR: ms 1..%u\n", CYL_T_MAX_MS);
    return;
  }
  for (int i = first; i <= last; i++) request(i, dir, (uint8_t)duty, (uint32_t)ms);
}

const test_module_t cylinder_module =
{
  .name    = "cyl",
  .summary = "xi lanh dien L (TIM4_CH3 PD14), R (TIM2_CH1 PA5)",
  .init    = cyl_init,
  .safe    = cyl_safe,
  .help    = cyl_help,
  .cmd     = cyl_cmd,
  .poll    = cyl_poll,
};
