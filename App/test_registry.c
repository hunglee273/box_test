/**
  * @file    test_registry.c
  * @brief   Bang cac module test va dieu phoi lenh.
  *          Them thiet bi moi: include header va them vao modules[].
  */
#include "test_registry.h"
#include "board_config.h"
#include "console.h"
#include <string.h>

#if TEST_EN_SERVO
#include "servo.h"
#endif
#if TEST_EN_OPTICAL
#include "optical.h"
#endif
#if TEST_EN_CYLINDER
#include "cylinder.h"
#endif
#if TEST_EN_STEPPER
#include "stepper.h"
#endif
#if TEST_EN_INA226
#include "ina226.h"
#endif

static const test_module_t *const modules[] =
{
#if TEST_EN_SERVO
  &servo_module,
#endif
#if TEST_EN_OPTICAL
  &optical_module,
#endif
#if TEST_EN_CYLINDER
  &cylinder_module,
#endif
#if TEST_EN_STEPPER
  &stepper_module,
#endif
#if TEST_EN_INA226
  &ina226_module,
#endif
  NULL
};

#define FOR_EACH_MODULE(m) \
  for (const test_module_t *const *pp = modules; ((m) = *pp) != NULL; pp++)

static const test_module_t *find_module(const char *name)
{
  const test_module_t *m;
  FOR_EACH_MODULE(m)
  {
    if (strcmp(m->name, name) == 0) return m;
  }
  return NULL;
}

static void print_list(void)
{
  const test_module_t *m;
  printf("Module:\n");
  FOR_EACH_MODULE(m)
  {
    printf("  %-10s %s\n", m->name, m->summary);
  }
}

static void print_help(void)
{
  printf("Lenh chung:\n"
         "  help [module]   huong dan chung / cua module\n"
         "  list            liet ke module\n"
         "  stop            dung moi tac vu, dua tat ca ve trang thai an toan\n"
         "Lenh module: <module> <lenh> [tham so], vd: servo set 1800\n");
  print_list();
}

void Tests_InitAll(void)
{
  const test_module_t *m;
  FOR_EACH_MODULE(m)
  {
    if (m->init) m->init();
  }
}

void Tests_PollAll(void)
{
  const test_module_t *m;
  uint32_t now = HAL_GetTick();
  FOR_EACH_MODULE(m)
  {
    if (m->poll) m->poll(now);
  }
}

void Tests_SafeAll(void)
{
  const test_module_t *m;
  FOR_EACH_MODULE(m)
  {
    if (m->safe) m->safe();
  }
}

void Tests_Exec(int argc, char *argv[])
{
  const test_module_t *m;

  if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0)
  {
    if (argc >= 2)
    {
      m = find_module(argv[1]);
      if (m && m->help) m->help();
      else printf("ERR: khong co module '%s'\n", argv[1]);
    }
    else
    {
      print_help();
    }
    return;
  }
  if (strcmp(argv[0], "list") == 0)
  {
    print_list();
    return;
  }
  if (strcmp(argv[0], "stop") == 0)
  {
    Tests_SafeAll();
    printf("OK: tat ca ve trang thai an toan\n");
    return;
  }

  m = find_module(argv[0]);
  if (m && m->cmd)
  {
    m->cmd(argc, argv);
  }
  else
  {
    printf("ERR: lenh '%s' khong ton tai, go 'help'\n", argv[0]);
  }
}
