/**
  * @file    console.c
  * @brief   Console lenh qua UART bang thanh ghi.
  *          RX: ngat + bo dem vong. TX: polling (printf -> __io_putchar).
  */
#include "console.h"
#include "board_config.h"
#include <stdlib.h>

#define RX_BUF_SIZE   256U  /* luy thua cua 2 */
#define LINE_BUF_SIZE 128U

static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;

static char     line[LINE_BUF_SIZE];
static uint16_t line_len;
static uint8_t  line_overflow;
static char     last_eol;

void Console_Init(void)
{
  CONSOLE_UART_CLK_ENABLE();
  CONSOLE_UART->CR1 = 0;
  CONSOLE_UART->BRR = (CONSOLE_UART_CLK_HZ + CONSOLE_BAUD / 2U) / CONSOLE_BAUD;
  CONSOLE_UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE | USART_CR1_UE;

  HAL_NVIC_SetPriority(CONSOLE_UART_IRQn, CONSOLE_IRQ_PRIO, 0);
  HAL_NVIC_EnableIRQ(CONSOLE_UART_IRQn);

  /* Khong dem stdout de printf ra ngay ca khi chua co '\n' */
  setvbuf(stdout, NULL, _IONBF, 0);
}

void CONSOLE_UART_IRQHandler(void)
{
  uint32_t isr = CONSOLE_UART->ISR;

  if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
  {
    CONSOLE_UART->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
  }
  if (isr & USART_ISR_RXNE_RXFNE)
  {
    uint8_t c = (uint8_t)(CONSOLE_UART->RDR & 0xFFU);
    uint16_t next = (uint16_t)((rx_head + 1U) & (RX_BUF_SIZE - 1U));
    if (next != rx_tail)  /* day thi bo ky tu */
    {
      rx_buf[rx_head] = c;
      rx_head = next;
    }
  }
}

/* Duoc _write trong syscalls.c goi cho moi ky tu cua printf */
int __io_putchar(int ch)
{
  if (ch == '\n')
  {
    while (!(CONSOLE_UART->ISR & USART_ISR_TXE_TXFNF)) {}
    CONSOLE_UART->TDR = '\r';
  }
  while (!(CONSOLE_UART->ISR & USART_ISR_TXE_TXFNF)) {}
  CONSOLE_UART->TDR = (uint8_t)ch;
  return ch;
}

static int rx_get(void)
{
  if (rx_tail == rx_head) return -1;
  uint8_t c = rx_buf[rx_tail];
  rx_tail = (uint16_t)((rx_tail + 1U) & (RX_BUF_SIZE - 1U));
  return c;
}

static int tokenize(char *s, char *argv[CONSOLE_MAX_ARGS])
{
  int argc = 0;
  while (*s && argc < CONSOLE_MAX_ARGS)
  {
    while (*s == ' ' || *s == '\t') *s++ = '\0';
    if (!*s) break;
    argv[argc++] = s;
    while (*s && *s != ' ' && *s != '\t') s++;
  }
  return argc;
}

int Console_ReadLine(int *argc, char *argv[CONSOLE_MAX_ARGS])
{
  int c;
  while ((c = rx_get()) >= 0)
  {
    if (c == '\r' || c == '\n')
    {
      /* Bo qua '\n' ngay sau '\r' (CRLF) va nguoc lai */
      char prev = last_eol;
      last_eol = (char)c;
      if (line_len == 0 && prev != 0 && prev != c) continue;
#if CONSOLE_ECHO
      printf("\n");
#endif
      if (line_overflow)
      {
        printf("ERR: dong lenh qua dai (toi da %u ky tu)\n", (unsigned)(LINE_BUF_SIZE - 1U));
        line_len = 0;
        line_overflow = 0;
        Console_Prompt();
        continue;
      }
      line[line_len] = '\0';
      line_len = 0;
      *argc = tokenize(line, argv);
      if (*argc == 0)
      {
        Console_Prompt();
        continue;
      }
      return 1;
    }

    last_eol = 0;
    if (c == '\b' || c == 0x7F)
    {
      if (line_len)
      {
        line_len--;
#if CONSOLE_ECHO
        printf("\b \b");
#endif
      }
      continue;
    }
    if (c < 0x20 || c > 0x7E) continue;

    if (line_len < LINE_BUF_SIZE - 1U)
    {
      line[line_len++] = (char)c;
#if CONSOLE_ECHO
      __io_putchar(c);
#endif
    }
    else
    {
      line_overflow = 1;
    }
  }
  return 0;
}

void Console_Prompt(void)
{
  printf("> ");
}

int Console_ParseInt(const char *s, int32_t *out)
{
  char *end;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0') return 0;
  *out = (int32_t)v;
  return 1;
}
