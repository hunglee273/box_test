/**
  * @file    stepper.c
  * @brief   3 dong co buoc tren TIM1 (CH1/CH3/CH4), 1 tick = 1 us.
  *
  *  TIM1 chay tu do (ARR = 0xFFFF). Moi kenh o che do Output Compare Toggle:
  *  moi lan khop CCRx chan PUL dao muc, ngat CC cong them nua chu ky vao
  *  CCRx. 2 lan dao = 1 buoc. Nho vay 3 kenh co tan so doc lap tren cung
  *  mot timer. Khi khong chay, kenh o che do Force Inactive (opto PUL tat).
  *  Toc do cap nhat moi buoc trong ngat: v += a/v (tang toc), v -= a/v
  *  (giam toc khi so buoc con lai <= v^2 / 2a).
  */
#include "stepper.h"
#include "console.h"
#include <string.h>
#include <strings.h>

#define TICK_HZ        1000000U
#define OCM_TOGGLE     0x3U
#define OCM_FORCE_LOW  0x4U
#define RUN_FOREVER    0xFFFFFFFFU

typedef struct
{
  const char   *name;
  uint8_t       ch;        /* kenh TIM1: 1..4 */
  GPIO_TypeDef *pul_port;
  uint16_t      pul_pin;
  GPIO_TypeDef *dir_port;
  uint16_t      dir_pin;
  GPIO_TypeDef *ena_port;
  uint16_t      ena_pin;
  GPIO_TypeDef *alm_port;
  uint16_t      alm_pin;
  uint8_t       dir_inv;   /* 1 = dao chieu quay */
} step_cfg_t;

typedef struct
{
  /* Dung chung voi ngat */
  volatile uint8_t  running;
  volatile uint8_t  finished;  /* bao cho poll() in ket qua */
  volatile uint32_t left;      /* so buoc con lai, RUN_FOREVER = chay lien tuc */
  volatile int32_t  pos;
  volatile uint32_t done;      /* so buoc da phat cua lenh hien tai */
  volatile float    v;         /* buoc/s hien tai */
  uint8_t           high;      /* muc PUL sau lan dao gan nhat */
  uint16_t          half;      /* nua chu ky, us */
  int8_t            dir;
  float             vmax;
  float             acc;

  /* Chi dung trong poll */
  uint8_t           ena;
  uint8_t           alarm;
  uint8_t           pending;   /* cho ENA on xong moi phat xung */
  uint32_t          t_start;
} step_state_t;

static const step_cfg_t cfg[STEP_COUNT] =
{
#define X(name, ch, pp, pn, dp, dn, ep, en, ap, an, inv) { #name, ch, pp, pn, dp, dn, ep, en, ap, an, inv },
  STEPPER_LIST(X)
#undef X
};

static step_state_t st[STEP_COUNT];

/* ---- Thanh ghi TIM1 theo kenh ------------------------------------------ */

static volatile uint32_t *ccr_reg(uint8_t ch)
{
  switch (ch)
  {
    case 1:  return &STEPPER_TIM->CCR1;
    case 2:  return &STEPPER_TIM->CCR2;
    case 3:  return &STEPPER_TIM->CCR3;
    default: return &STEPPER_TIM->CCR4;
  }
}

static void set_ocmode(uint8_t ch, uint32_t mode)
{
  volatile uint32_t *ccmr = (ch <= 2) ? &STEPPER_TIM->CCMR1 : &STEPPER_TIM->CCMR2;
  uint32_t sh = (ch & 1U) ? 0U : 8U;
  uint32_t clr = (0xFFUL << sh) | (1UL << (sh + 16U));  /* CCxS, OCxFE/PE/M, OCxM[3] */
  uint32_t set = ((mode & 7U) << (sh + 4U)) | (((mode >> 3) & 1U) << (sh + 16U));
  *ccmr = (*ccmr & ~clr) | set;
}

static uint32_t cc_flag(uint8_t ch)
{
  return 1UL << ch;  /* CCxIF trong SR va CCxIE trong DIER cung vi tri */
}

static uint16_t half_period(float v)
{
  float h = (float)TICK_HZ / (2.0f * v);
  if (h > 65000.0f) h = 65000.0f;
  if (h < 10.0f) h = 10.0f;
  return (uint16_t)h;
}

/* ---------------------------------------------------------------------- */

static void out_stop(int i)
{
  uint8_t ch = cfg[i].ch;
  STEPPER_TIM->DIER &= ~cc_flag(ch);
  set_ocmode(ch, OCM_FORCE_LOW);
  STEPPER_TIM->SR = ~cc_flag(ch);
  st[i].high = 0;
  st[i].running = 0;
}

void STEPPER_TIM_CC_IRQHandler(void)
{
  uint32_t sr = STEPPER_TIM->SR & STEPPER_TIM->DIER;

  for (int i = 0; i < STEP_COUNT; i++)
  {
    step_state_t *m = &st[i];
    uint8_t ch = cfg[i].ch;
    if (!(sr & cc_flag(ch))) continue;
    STEPPER_TIM->SR = ~cc_flag(ch);

    m->high ^= 1U;
    if (!m->high)
    {
      /* Canh xuong: xong 1 buoc */
      m->pos += m->dir;
      m->done++;
      if (m->left != RUN_FOREVER && --m->left == 0)
      {
        out_stop(i);
        m->finished = 1;
        continue;
      }
      float v = m->v;
      if (m->left != RUN_FOREVER && (float)m->left <= v * v / (2.0f * m->acc))
      {
        v -= m->acc / v;
        if (v < STEPPER_SPS_START) v = STEPPER_SPS_START;
      }
      else if (v < m->vmax)
      {
        v += m->acc / v;
        if (v > m->vmax) v = m->vmax;
      }
      else if (v > m->vmax)
      {
        v -= m->acc / v;   /* giam toc khi ha vmax luc dang chay */
        if (v < m->vmax) v = m->vmax;
      }
      m->v = v;
      m->half = half_period(v);
    }
    *ccr_reg(ch) = (uint16_t)(*ccr_reg(ch) + m->half);
  }
}

/* Muc chan de opto dau vao driver dan / tat */
static GPIO_PinState opto_level(uint8_t on)
{
#if STEPPER_COMMON_ANODE
  return on ? GPIO_PIN_RESET : GPIO_PIN_SET;
#else
  return on ? GPIO_PIN_SET : GPIO_PIN_RESET;
#endif
}

static uint8_t opto_is_on(GPIO_TypeDef *port, uint16_t pin)
{
  return (port->ODR & pin) ? (opto_level(1) == GPIO_PIN_SET) : (opto_level(1) == GPIO_PIN_RESET);
}

static uint8_t read_alarm(int i)
{
  return HAL_GPIO_ReadPin(cfg[i].alm_port, cfg[i].alm_pin) == STEPPER_ALM_ACTIVE_LEVEL;
}

static void write_ena(int i, uint8_t on)
{
  uint8_t opto = STEPPER_ENA_OPTO_DISABLES ? !on : on;
  HAL_GPIO_WritePin(cfg[i].ena_port, cfg[i].ena_pin, opto_level(opto));
  st[i].ena = on;
}

static uint8_t ena_from_pin(int i)
{
  uint8_t opto = opto_is_on(cfg[i].ena_port, cfg[i].ena_pin);
  return STEPPER_ENA_OPTO_DISABLES ? !opto : opto;
}

static void write_dir(int i, int8_t dir)
{
  uint8_t opto = (dir < 0) ^ cfg[i].dir_inv;
  HAL_GPIO_WritePin(cfg[i].dir_port, cfg[i].dir_pin, opto_level(opto));
}

static void out_start(int i)
{
  step_state_t *m = &st[i];
  uint8_t ch = cfg[i].ch;

  m->half = half_period(m->v);
  m->high = 0;
  m->running = 1;
  __disable_irq();
  *ccr_reg(ch) = (uint16_t)(STEPPER_TIM->CNT + m->half);
  STEPPER_TIM->SR = ~cc_flag(ch);
  set_ocmode(ch, OCM_TOGGLE);
  STEPPER_TIM->DIER |= cc_flag(ch);
  __enable_irq();
}

static int prepare(int i, int8_t dir, uint32_t left, uint32_t sps, uint32_t acc)
{
  step_state_t *m = &st[i];

  if (m->running || m->pending)
  {
    printf("ERR: %s dang chay, 'step %s stop' truoc\n", cfg[i].name, cfg[i].name);
    return -1;
  }
#if STEPPER_ALM_STOP
  if (read_alarm(i))
  {
    printf("ERR: %s dang bao ALM (chan %s). Sai muc thi doi STEPPER_ALM_ACTIVE_LEVEL\n",
           cfg[i].name, STEPPER_ALM_ACTIVE_LEVEL == GPIO_PIN_RESET ? "LOW" : "HIGH");
    return -1;
  }
#endif
  if (sps < STEPPER_SPS_MIN) sps = STEPPER_SPS_MIN;
  if (sps > STEPPER_SPS_MAX) sps = STEPPER_SPS_MAX;
  if (acc == 0) acc = STEPPER_ACC_DEFAULT;

  /* DIR dat truoc canh PUL dau tien it nhat nua chu ky (>= 10 us) */
  write_dir(i, dir);

  m->dir = dir;
  m->left = left;
  m->done = 0;
  m->vmax = (float)sps;
  m->acc = (float)acc;
  m->v = (sps < STEPPER_SPS_START) ? (float)sps : (float)STEPPER_SPS_START;
  m->finished = 0;
  m->t_start = HAL_GetTick();

  if (!m->ena)
  {
    write_ena(i, 1);
    m->pending = 1;  /* poll() se goi out_start sau STEPPER_ENA_DELAY_MS */
    printf("[step] %s: bat ENA, cho %u ms\n", cfg[i].name, STEPPER_ENA_DELAY_MS);
  }
  else
  {
    out_start(i);
  }
  return 0;
}

/* ---- API --------------------------------------------------------------- */

int Step_Move(step_id_t id, int32_t steps, uint32_t sps, uint32_t acc)
{
  if (id >= STEP_COUNT || steps == 0) return -1;
  uint32_t n = (uint32_t)(steps < 0 ? -steps : steps);
  if (n > STEPPER_MOVE_MAX) n = STEPPER_MOVE_MAX;
  return prepare(id, steps > 0 ? +1 : -1, n, sps, acc);
}

int Step_Run(step_id_t id, int32_t sps, uint32_t acc)
{
  if (id >= STEP_COUNT || sps == 0) return -1;
  return prepare(id, sps > 0 ? +1 : -1, RUN_FOREVER,
                 (uint32_t)(sps < 0 ? -sps : sps), acc);
}

void Step_Stop(step_id_t id)
{
  if (id >= STEP_COUNT) return;
  step_state_t *m = &st[id];
  if (m->pending)
  {
    m->pending = 0;
    return;
  }
  if (!m->running) return;

  __disable_irq();
  uint32_t need = (uint32_t)(m->v * m->v / (2.0f * m->acc)) + 1U;
  if (m->left > need) m->left = need;
  __enable_irq();
}

void Step_Halt(step_id_t id)
{
  if (id >= STEP_COUNT) return;
  st[id].pending = 0;
  __disable_irq();
  if (st[id].running) out_stop(id);
  __enable_irq();
}

void Step_Enable(step_id_t id, uint8_t on)
{
  if (id >= STEP_COUNT) return;
  if (!on) Step_Halt(id);
  write_ena(id, on);
}

uint8_t Step_IsBusy(step_id_t id)
{
  return (id < STEP_COUNT) && (st[id].running || st[id].pending);
}

int32_t Step_GetPos(step_id_t id)
{
  return (id < STEP_COUNT) ? st[id].pos : 0;
}

uint8_t Step_IsAlarm(step_id_t id)
{
  return (id < STEP_COUNT) && read_alarm(id);
}

/* ---- Module ------------------------------------------------------------ */

static void step_init(void)
{
  /* Cau hinh lai TIM1 tu PWM (CubeMX) sang Output Compare Toggle */
  STEPPER_TIM->CR1 &= ~TIM_CR1_CEN;
  STEPPER_TIM->DIER = 0;
  STEPPER_TIM->PSC = STEPPER_TIM_CLK_HZ / TICK_HZ - 1U;
  STEPPER_TIM->ARR = 0xFFFFU;
  for (int i = 0; i < STEP_COUNT; i++)
  {
    uint8_t ch = cfg[i].ch;
#if STEPPER_COMMON_ANODE
    /* Open-drain: tha chan = opto tat, khong co dong ro tu 5 V ve 3.3 V */
    cfg[i].pul_port->OTYPER |= cfg[i].pul_pin;
    cfg[i].dir_port->OTYPER |= cfg[i].dir_pin;
    cfg[i].ena_port->OTYPER |= cfg[i].ena_pin;
#endif
    set_ocmode(ch, OCM_FORCE_LOW);
    STEPPER_TIM->CCER &= ~(0xFUL << ((ch - 1U) * 4U));   /* khong dung CCxN */
#if STEPPER_COMMON_ANODE
    STEPPER_TIM->CCER |= 2UL << ((ch - 1U) * 4U);        /* CCxP = 1: nghi = HIGH = opto tat */
#endif
    STEPPER_TIM->CCER |= 1UL << ((ch - 1U) * 4U);        /* CCxE = 1 */
  }
  STEPPER_TIM->BDTR |= TIM_BDTR_MOE;
  STEPPER_TIM->EGR = TIM_EGR_UG;
  STEPPER_TIM->SR = 0;
  STEPPER_TIM->CR1 |= TIM_CR1_CEN;

  HAL_NVIC_SetPriority(STEPPER_TIM_CC_IRQn, STEPPER_IRQ_PRIO, 0);
  HAL_NVIC_EnableIRQ(STEPPER_TIM_CC_IRQn);

  for (int i = 0; i < STEP_COUNT; i++)
  {
    /* Giu muc ENA ma CubeMX dat luc khoi dong */
    st[i].ena = ena_from_pin(i);
    st[i].alarm = read_alarm(i);
    if (st[i].alarm)
    {
      printf("[step] %s: ALM dang bao loi luc khoi dong\n", cfg[i].name);
    }
  }
}

/* Dung xung ngay nhung GIU ENA: truc V tha ENA co the bi roi tai */
static void step_safe(void)
{
  for (int i = 0; i < STEP_COUNT; i++) Step_Halt((step_id_t)i);
}

static void step_poll(uint32_t now)
{
  for (int i = 0; i < STEP_COUNT; i++)
  {
    step_state_t *m = &st[i];

    if (m->pending && (now - m->t_start) >= STEPPER_ENA_DELAY_MS)
    {
      m->pending = 0;
      out_start(i);
    }

    if (m->finished)
    {
      m->finished = 0;
      printf("[step] %s xong %lu buoc trong %lu ms, pos=%ld\n", cfg[i].name,
             (unsigned long)m->done, (unsigned long)(now - m->t_start), (long)m->pos);
    }

    uint8_t alm = read_alarm(i);
    if (alm != m->alarm)
    {
      m->alarm = alm;
      if (STEPPER_ALM_STOP && alm && (m->running || m->pending))
      {
        Step_Halt((step_id_t)i);
        printf("[step] %s ALM! da dung ngay, pos=%ld\n", cfg[i].name, (long)m->pos);
      }
      else
      {
        printf("[step] %s ALM %s\n", cfg[i].name, alm ? "BAO LOI" : "het");
      }
    }
  }
}

static void step_help(void)
{
  printf("step                           trang thai ca 3 dong co\n"
         "step <ten> move <buoc> [sps] [acc]  chay N buoc (am = nguoc chieu)\n"
         "step <ten> rev <vong> [vong/s] chay theo so vong (PPR = %u)\n"
         "step <ten> run <+-sps> [acc]   chay lien tuc den khi stop\n"
         "step <ten|all> stop            giam toc roi dung\n"
         "step <ten|all> halt            dung ngay khong ramp\n"
         "step <ten|all> ena on|off      bat/tat driver (off = tha truc)\n"
         "step <ten> zero                dat pos = 0\n"
         "step io                        muc chan thuc te (kiem tra day)\n"
         "ten: H1 (PE14), V (PE13), H2 (PE9). sps %u..%u, mac dinh %u, acc %u\n",
         STEPPER_PPR, STEPPER_SPS_MIN, STEPPER_SPS_MAX, STEPPER_SPS_DEFAULT, STEPPER_ACC_DEFAULT);
}

static void print_status(void)
{
  for (int i = 0; i < STEP_COUNT; i++)
  {
    step_state_t *m = &st[i];
    const char *state = m->pending ? "cho ENA" : (m->running ? "chay" : "dung");
    printf("[step] %-2s %-7s pos=%-8ld v=%-5lu ENA=%s(%s) ALM=%s(%s)\n",
           cfg[i].name, state, (long)m->pos,
           (unsigned long)(m->running ? m->v : 0.0f),
           m->ena ? "on " : "off",
           HAL_GPIO_ReadPin(cfg[i].ena_port, cfg[i].ena_pin) == GPIO_PIN_SET ? "H" : "L",
           read_alarm(i) ? "LOI" : "ok ",
           HAL_GPIO_ReadPin(cfg[i].alm_port, cfg[i].alm_pin) == GPIO_PIN_SET ? "H" : "L");
  }
}

static const char *hl(GPIO_TypeDef *port, uint16_t pin)
{
  return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET ? "H" : "L";
}

static void print_io(void)
{
  printf("Wiring: %s. Opto ON khi chan %s.\n",
         STEPPER_COMMON_ANODE ? "anode chung, open-drain" : "cathode chung, push-pull",
         opto_level(1) == GPIO_PIN_SET ? "HIGH" : "LOW");
  for (int i = 0; i < STEP_COUNT; i++)
  {
    printf("[step] %-2s PUL=%s DIR=%s(opto %s) ENA=%s(opto %s -> driver %s) ALM=%s(%s)\n",
           cfg[i].name, hl(cfg[i].pul_port, cfg[i].pul_pin),
           hl(cfg[i].dir_port, cfg[i].dir_pin),
           opto_is_on(cfg[i].dir_port, cfg[i].dir_pin) ? "on " : "off",
           hl(cfg[i].ena_port, cfg[i].ena_pin),
           opto_is_on(cfg[i].ena_port, cfg[i].ena_pin) ? "on " : "off",
           ena_from_pin(i) ? "BAT" : "TAT",
           hl(cfg[i].alm_port, cfg[i].alm_pin), read_alarm(i) ? "LOI" : "ok");
  }
}

static void step_cmd(int argc, char *argv[])
{
  if (argc < 3)
  {
    if (argc == 2 && strcmp(argv[1], "help") == 0) step_help();
    else if (argc == 2 && strcmp(argv[1], "io") == 0) print_io();
    else print_status();
    return;
  }

  int first = -1, last = -1;
  if (strcasecmp(argv[1], "all") == 0)
  {
    first = 0;
    last = STEP_COUNT - 1;
  }
  else
  {
    for (int i = 0; i < STEP_COUNT; i++)
    {
      if (strcasecmp(argv[1], cfg[i].name) == 0) first = last = i;
    }
  }
  if (first < 0)
  {
    printf("ERR: khong co dong co '%s'\n", argv[1]);
    return;
  }

  const char *sub = argv[2];
  int32_t a, b, c;

  if (strcmp(sub, "stop") == 0)
  {
    for (int i = first; i <= last; i++) Step_Stop((step_id_t)i);
  }
  else if (strcmp(sub, "halt") == 0)
  {
    for (int i = first; i <= last; i++) Step_Halt((step_id_t)i);
    print_status();
  }
  else if (strcmp(sub, "ena") == 0 && argc >= 4)
  {
    uint8_t on = (strcmp(argv[3], "on") == 0);
    for (int i = first; i <= last; i++) Step_Enable((step_id_t)i, on);
    print_status();
  }
  else if (first != last)
  {
    printf("ERR: move/run/zero chi dung cho 1 dong co\n");
  }
  else if (strcmp(sub, "zero") == 0)
  {
    st[first].pos = 0;
    print_status();
  }
  else if (strcmp(sub, "move") == 0 && argc >= 4 && Console_ParseInt(argv[3], &a))
  {
    b = STEPPER_SPS_DEFAULT;
    c = STEPPER_ACC_DEFAULT;
    if ((argc >= 5 && !Console_ParseInt(argv[4], &b)) ||
        (argc >= 6 && !Console_ParseInt(argv[5], &c)) || b <= 0 || c <= 0 || a == 0)
    {
      printf("ERR: step %s move <buoc khac 0> [sps>0] [acc>0]\n", cfg[first].name);
      return;
    }
    if (Step_Move((step_id_t)first, a, (uint32_t)b, (uint32_t)c) == 0)
    {
      printf("[step] %s move %ld buoc, %lu sps, acc %lu\n", cfg[first].name,
             (long)a, (unsigned long)st[first].vmax, (unsigned long)st[first].acc);
    }
  }
  else if (strcmp(sub, "rev") == 0 && argc >= 4 && Console_ParseInt(argv[3], &a))
  {
    b = 1;
    if ((argc >= 5 && !Console_ParseInt(argv[4], &b)) || b <= 0 || a == 0 ||
        a > STEPPER_MOVE_MAX / STEPPER_PPR || a < -(STEPPER_MOVE_MAX / STEPPER_PPR))
    {
      printf("ERR: step %s rev <vong khac 0> [vong/s>0]\n", cfg[first].name);
      return;
    }
    if (Step_Move((step_id_t)first, a * STEPPER_PPR, (uint32_t)(b * STEPPER_PPR),
                  STEPPER_ACC_DEFAULT) == 0)
    {
      printf("[step] %s %ld vong = %ld buoc, %lu sps\n", cfg[first].name, (long)a,
             (long)(a * STEPPER_PPR), (unsigned long)st[first].vmax);
    }
  }
  else if (strcmp(sub, "run") == 0 && argc >= 4 && Console_ParseInt(argv[3], &a))
  {
    c = STEPPER_ACC_DEFAULT;
    if ((argc >= 5 && !Console_ParseInt(argv[4], &c)) || c <= 0 || a == 0)
    {
      printf("ERR: step %s run <+-sps> [acc>0]\n", cfg[first].name);
      return;
    }
    if (Step_Run((step_id_t)first, a, (uint32_t)c) == 0)
    {
      printf("[step] %s run %ld sps, 'step %s stop' de dung\n",
             cfg[first].name, (long)a, cfg[first].name);
    }
  }
  else
  {
    step_help();
  }
}

const test_module_t stepper_module =
{
  .name    = "step",
  .summary = "3 dong co buoc H1/V/H2 (TIM1 CH4/CH3/CH1)",
  .init    = step_init,
  .safe    = step_safe,
  .help    = step_help,
  .cmd     = step_cmd,
  .poll    = step_poll,
};
