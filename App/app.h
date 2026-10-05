/**
  * @file    app.h
  * @brief   Diem vao cua firmware test, duoc goi tu main.c.
  */
#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

void App_Init(void);  /* goi sau khi MX_xxx_Init() xong */
void App_Poll(void);  /* goi lien tuc trong while(1) */

#ifdef __cplusplus
}
#endif

#endif /* APP_H */
