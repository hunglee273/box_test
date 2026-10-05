/**
  * @file    test_registry.h
  * @brief   Bang cac module test va dieu phoi lenh.
  */
#ifndef TEST_REGISTRY_H
#define TEST_REGISTRY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  const char *name;                     /* ten lenh, vd "servo" */
  const char *summary;                  /* mo ta 1 dong cho lenh list */
  void (*init)(void);                   /* goi 1 lan khi khoi dong */
  void (*safe)(void);                   /* dua thiet bi ve trang thai an toan */
  void (*help)(void);                   /* in huong dan chi tiet */
  void (*cmd)(int argc, char *argv[]);  /* argv[0] = name */
  void (*poll)(uint32_t now_ms);        /* goi lien tuc, khong duoc blocking */
} test_module_t;

void Tests_InitAll(void);
void Tests_PollAll(void);
void Tests_SafeAll(void);
void Tests_Exec(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* TEST_REGISTRY_H */
