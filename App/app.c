/**
  * @file    app.c
  * @brief   Diem vao cua firmware test.
  */
#include "app.h"
#include "console.h"
#include "test_registry.h"

void App_Init(void)
{
  Console_Init();
  printf("\n=== MCU TEST FIRMWARE ===\n"
         "Go 'help' de xem lenh, 'stop' de dung tat ca.\n");
  Tests_InitAll();
  Console_Prompt();
}

void App_Poll(void)
{
  int argc;
  char *argv[CONSOLE_MAX_ARGS];

  if (Console_ReadLine(&argc, argv))
  {
    Tests_Exec(argc, argv);
    Console_Prompt();
  }
  Tests_PollAll();
}
