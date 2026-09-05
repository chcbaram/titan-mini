#include "uart.h"
#include "qbuffer.h"
#include "cli.h"

#ifdef _USE_HW_UART

/*
 * SCI2 — 회로도 13페이지의 H1 커넥터
 *
 *   TXD2  P801
 *   RXD2  P802
 *
 * HSLink 디버거의 가상 시리얼이 여기 붙어 있어 케이블 하나로 적재와 콘솔이
 * 동시에 된다. 회로도에는 H1 이 DNP 로 표기돼 있지만 실물에는 배선돼 있다.
 *
 * 40핀 헤더의 SCI1(P707 TXD1 / P706 RXD1)도 UART 로 쓸 수 있다. 옮기려면
 * configuration.xml 의 채널과 핀을 바꾸고 재생성한다. (docs/21-uart-cli.md)
 */

#define UART_RX_BUF_LENGTH        1024


typedef struct
{
  bool     is_open;
  uint32_t baud;

  uint8_t   rx_buf[UART_RX_BUF_LENGTH];
  qbuffer_t qbuffer;

  volatile bool tx_done;

  uint32_t rx_cnt;
  uint32_t tx_cnt;
} uart_tbl_t;

typedef struct
{
  const char            *p_msg;
  uart_instance_t const *p_uart;
  uart_driver_t         *p_driver;
} uart_hw_t;


#if CLI_USE(HW_UART)
static void cliUart(cli_args_t *args);
#endif


static bool is_init = false;

static uart_tbl_t uart_tbl[UART_MAX_CH];

static uart_hw_t uart_hw_tbl[UART_MAX_CH] =
  {
    {"SCI2 HSLink  ", &g_uart2, NULL},   // P801 TXD2 / P802 RXD2 (H1 커넥터)
  };




bool uartInit(void)
{
  for (int i = 0; i < UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;
    uart_tbl[i].baud    = 115200;
    uart_tbl[i].tx_done = true;
    uart_tbl[i].rx_cnt  = 0;
    uart_tbl[i].tx_cnt  = 0;
  }

  is_init = true;

#if CLI_USE(HW_UART)
  cliAdd("uart", cliUart);
#endif

  return true;
}

bool uartDeInit(void)
{
  return true;
}

bool uartIsInit(void)
{
  return is_init;
}

bool uartSetDriver(uint8_t ch, uart_driver_t *p_driver)
{
  if (ch >= UART_MAX_CH) return false;

  uart_hw_tbl[ch].p_driver = p_driver;
  return true;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  bool ret = false;

  if (ch >= UART_MAX_CH) return false;

  if (uart_tbl[ch].is_open == true && uart_tbl[ch].baud == baud)
  {
    return true;
  }

  if (uart_hw_tbl[ch].p_driver != NULL)
  {
    ret = uart_hw_tbl[ch].p_driver->open(baud);
    uart_tbl[ch].is_open = ret;
    uart_tbl[ch].baud    = baud;
    return ret;
  }

  uart_instance_t const *p_uart = uart_hw_tbl[ch].p_uart;
  fsp_err_t              err;

  qbufferCreate(&uart_tbl[ch].qbuffer, uart_tbl[ch].rx_buf, UART_RX_BUF_LENGTH);

  if (uart_tbl[ch].is_open == true)
  {
    p_uart->p_api->close(p_uart->p_ctrl);
    uart_tbl[ch].is_open = false;
  }

  err = p_uart->p_api->open(p_uart->p_ctrl, p_uart->p_cfg);
  if (err != FSP_SUCCESS)
  {
    return false;
  }

  //-- ra_gen 의 g_uart1_cfg 는 115200 으로 고정돼 있다. 다른 속도를 요구하면
  //   분주값을 다시 계산해서 넣는다.
  //
  if (baud != 115200)
  {
    sci_b_baud_setting_t baud_setting;

    err = R_SCI_B_UART_BaudCalculate(baud, false, 5000, &baud_setting);
    if (err == FSP_SUCCESS)
    {
      err = p_uart->p_api->baudSet(p_uart->p_ctrl, &baud_setting);
    }
    if (err != FSP_SUCCESS)
    {
      p_uart->p_api->close(p_uart->p_ctrl);
      return false;
    }
  }

  uart_tbl[ch].is_open = true;
  uart_tbl[ch].baud    = baud;
  uart_tbl[ch].tx_done = true;

  return true;
}

bool uartIsOpen(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  return uart_tbl[ch].is_open;
}

bool uartClose(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  if (uart_hw_tbl[ch].p_driver != NULL)
  {
    uart_tbl[ch].is_open = false;
    return uart_hw_tbl[ch].p_driver->close();
  }

  if (uart_tbl[ch].is_open == true)
  {
    uart_instance_t const *p_uart = uart_hw_tbl[ch].p_uart;

    p_uart->p_api->close(p_uart->p_ctrl);
    uart_tbl[ch].is_open = false;
  }

  return true;
}

uint32_t uartAvailable(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  if (uart_hw_tbl[ch].p_driver != NULL)
  {
    return uart_hw_tbl[ch].p_driver->available();
  }

  return qbufferAvailable(&uart_tbl[ch].qbuffer);
}

bool uartFlush(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  if (uart_hw_tbl[ch].p_driver != NULL)
  {
    return uart_hw_tbl[ch].p_driver->flush();
  }

  qbufferFlush(&uart_tbl[ch].qbuffer);
  return true;
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;

  if (ch >= UART_MAX_CH) return 0;

  if (uart_hw_tbl[ch].p_driver != NULL)
  {
    return uart_hw_tbl[ch].p_driver->read();
  }

  qbufferRead(&uart_tbl[ch].qbuffer, &ret, 1);
  return ret;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  if (ch >= UART_MAX_CH) return 0;
  if (length == 0) return 0;

  if (uart_hw_tbl[ch].p_driver != NULL)
  {
    return uart_hw_tbl[ch].p_driver->write(p_data, length);
  }

  if (uart_tbl[ch].is_open != true) return 0;

  uart_instance_t const *p_uart = uart_hw_tbl[ch].p_uart;

  //-- FSP 의 write 는 인터럽트 구동이라 바로 리턴한다. 다음 호출이 진행 중인
  //   전송을 덮어쓰지 않도록 여기서 완료를 기다린다.
  //
  uart_tbl[ch].tx_done = false;

  if (p_uart->p_api->write(p_uart->p_ctrl, p_data, length) != FSP_SUCCESS)
  {
    uart_tbl[ch].tx_done = true;
    return 0;
  }

  //-- 최악의 경우(1바이트당 10비트)보다 넉넉하게 잡은 타임아웃.
  //   전송이 끝나지 않아도 영원히 멈추지는 않게 한다.
  //
  uint32_t timeout = 100 + (length * 10 * 1000) / uart_tbl[ch].baud;
  uint32_t pre_time = millis();

  while (uart_tbl[ch].tx_done == false)
  {
    if (millis() - pre_time >= timeout)
    {
      return 0;
    }
  }

  uart_tbl[ch].tx_cnt += length;
  return length;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;

  va_start(args, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (len <= 0) return 0;
  if (len > (int)sizeof(buf)) len = (int)sizeof(buf);

  return uartWrite(ch, (uint8_t *)buf, (uint32_t)len);
}

uint32_t uartGetBaud(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].baud;
}

uint32_t uartGetRxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].rx_cnt;
}

uint32_t uartGetTxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].tx_cnt;
}

//-- ra_gen/hal_data.c 가 이 이름으로 콜백을 등록한다.
//
void uartCallback2(uart_callback_args_t *p_args)
{
  const uint8_t ch = _DEF_UART1;

  switch (p_args->event)
  {
    case UART_EVENT_RX_CHAR:
    {
      uint8_t rx_data = (uint8_t)p_args->data;

      qbufferWrite(&uart_tbl[ch].qbuffer, &rx_data, 1);
      uart_tbl[ch].rx_cnt++;
      break;
    }

    case UART_EVENT_TX_COMPLETE:
      uart_tbl[ch].tx_done = true;
      break;

    default:
      break;
  }
}


#if CLI_USE(HW_UART)
void cliUart(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i = 0; i < UART_MAX_CH; i++)
    {
      cliPrintf("_DEF_UART%d : %s %d bps",
                i + 1, uart_hw_tbl[i].p_msg, uartGetBaud(i));
      cliPrintf("  rx %d, tx %d\n", uartGetRxCnt(i), uartGetTxCnt(i));
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "test"))
  {
    uint8_t ch = (uint8_t)(args->getData(1) - 1);

    if (ch >= UART_MAX_CH)
    {
      cliPrintf("ch %d 는 범위 밖이다\n", ch + 1);
      return;
    }

    while (cliKeepLoop())
    {
      if (uartAvailable(ch) > 0)
      {
        uartPrintf(ch, "%c", uartRead(ch));
      }
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("uart info\n");
    cliPrintf("uart test ch[1~%d]\n", HW_UART_MAX_CH);
  }
}
#endif

#endif
