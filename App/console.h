/**
  * @file    console.h
  * @brief   Console lenh qua UART: nhan theo dong, tach tham so, printf.
  */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_MAX_ARGS 8

void Console_Init(void);

/* Tra ve 1 khi co mot dong lenh hoan chinh. argv tro vao bo dem noi bo,
   chi hop le den lan goi tiep theo. */
int  Console_ReadLine(int *argc, char *argv[CONSOLE_MAX_ARGS]);

void Console_Prompt(void);

/* Doc so nguyen (thap phan, co dau +/-). Tra ve 1 neu hop le. */
int  Console_ParseInt(const char *s, int32_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_H */
