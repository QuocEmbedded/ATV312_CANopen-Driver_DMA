#include "stm32f407xx.h"
#include "uart.h"

/******************** PRIVATE VARIABLES ********************/
static char rx_raw_buffer[UART_RX_BUFF_SIZE];
static uint8_t app_rx_buffer[UART_RX_BUFF_SIZE];

static volatile uint8_t  s_rx_ready = 0;
static volatile uint16_t s_rx_len   = 0;
static volatile uint16_t rx_index   = 0;

/******************** FUNCTIONS ********************/

void UART1_Init(void)
{
    // 1. Clock GPIOA & USART1
    RCC->AHB1ENR |= (1U << 0);
    RCC->APB2ENR |= (1U << 4);

    // 2. Cấu hình chân PA9 (TX), PA10 (RX) Alternate Function AF7
    GPIOA->MODER &= ~((3U << 18) | (3U << 20));
    GPIOA->MODER |=  ((2U << 18) | (2U << 20));

    GPIOA->OSPEEDR |= ((2U << 18) | (2U << 20)); // High speed

    GPIOA->AFR[1] &= ~((0xFU << 4) | (0xFU << 8));
    GPIOA->AFR[1] |=  ((7U << 4)  | (7U << 8));

    // 3. Cấu hình USART1
    USART1->CR1 &= ~(1U << 13); // Tắt UE
    USART1->BRR  = 0x2D9;       // 115200 @ 84MHz (APB2)

    // Bật ngắt RXNEIE (bit 5) và IDLEIE (bit 4)
    USART1->CR1 |= (1U << 5) | (1U << 4);

    // Bật TE, RE và UE
    USART1->CR1 |= (3U << 2) | (1U << 13);

    // 4. NVIC
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);
}

uint16_t UART1_ReadPacket(uint8_t *dest, uint16_t max_len)
{
    // Kiểm tra an toàn: không có data hoặc mảng đích kích thước 0
    if (!s_rx_ready || max_len == 0)
    {
        return 0;
    }

    uint16_t len = (s_rx_len < max_len) ? s_rx_len : max_len;
    for (uint16_t i = 0; i < len; i++)
    {
        dest[i] = app_rx_buffer[i];
    }

    if (len < max_len)
    {
        dest[len] = '\0';
    }
    else
    {
        dest[max_len - 1] = '\0';
    }

    s_rx_ready = 0;
    return len;
}

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;

    // 1. Giai đoạn lấy từ shifter tới DR, gom vào rx_raw_buffer
    if (sr & USART_SR_RXNE)
    {
        char c = (char)(USART1->DR); // Luôn đọc DR để xóa cờ RXNE

        if (rx_index < (UART_RX_BUFF_SIZE - 1))
        {
            rx_raw_buffer[rx_index++] = c;
        }
    }

    // 2. Giai đoạn kiểm tra sau khi đã gửi xong gói tin (IDLE)
    if (sr & USART_SR_IDLE)
    {
        // Xóa cờ ngắt IDLE: đọc SR rồi đọc DR
        (void)USART1->SR;
        (void)USART1->DR;

        if (rx_index > 0 && !s_rx_ready)
        {
            s_rx_len = rx_index;
            for (uint8_t i = 0; i < s_rx_len; i++)
            {
                app_rx_buffer[i] = (uint8_t)rx_raw_buffer[i];
            }
            app_rx_buffer[s_rx_len] = '\0';

            s_rx_ready = 1;
            rx_index = 0;
        }
        else if (s_rx_ready)
        {
            rx_index = 0; // Reset để đón chu kỳ sau nếu app chưa xử lý kịp
        }
    }

    // 3. Xóa cờ lỗi Overrun nếu có
    if (sr & USART_SR_ORE)
    {
        (void)USART1->SR;
        (void)USART1->DR;
    }
}

void Send_String(const char *tx_ptr)
{
    while (*tx_ptr != '\0')
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = *tx_ptr;
        tx_ptr++;
    }
}

/**
 * @brief Chuyển đổi số nguyên có dấu thành chuỗi ký tự ASCII
 * @param num  Số cần chuyển
 * @param str  Con trỏ bộ đệm chứa chuỗi kết quả
 */
void Int_To_String(int32_t num, char *str)
{
    int i = 0;
    uint8_t is_negative = 0;

    /* Xử lý trường hợp số 0 */
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    /* Xử lý số âm */
    if (num < 0)
    {
        is_negative = 1;
        num = -num;
    }

    /* Tách từng chữ số từ hàng đơn vị ngược lên */
    while (num > 0)
    {
        str[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative)
    {
        str[i++] = '-';
    }

    str[i] = '\0';

    /* Đảo ngược chuỗi lại cho đúng thứ tự */
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void UART_Print_Hex16(uint16_t val)
{
    const char hex_chars[] = "0123456789ABCDEF";
    char str[7];
    str[0] = '0';
    str[1] = 'x';
    str[2] = hex_chars[(val >> 12) & 0x0F];
    str[3] = hex_chars[(val >> 8)  & 0x0F];
    str[4] = hex_chars[(val >> 4)  & 0x0F];
    str[5] = hex_chars[val & 0x0F];
    str[6] = '\0';
    Send_String(str);
}

/**
 * @brief In số thực có 1 chữ số thập phân ra chuỗi (ví dụ 30.5)
 * @param val  Giá trị float cần in
 * @param str  Buffer chứa chuỗi kết quả
 */
void Float_To_String_1Dec(float val, char *str)
{
    // Xử lý số âm nếu có
    if (val < 0.0f)
    {
        *str++ = '-';
        val = -val;
    }

    // Tách phần nguyên và phần thập phân (1 chữ số sau dấu phẩy)
    int32_t int_part = (int32_t)val;
    int32_t dec_part = (int32_t)((val - (float)int_part) * 10.0f + 0.5f); // Làm tròn

    if (dec_part >= 10)
    {
        int_part += 1;
        dec_part = 0;
    }

    // Đổi phần nguyên
    char temp[16];
    Int_To_String(int_part, temp);

    // Ghép phần nguyên
    char *p = temp;
    while (*p != '\0')
    {
        *str++ = *p++;
    }

    // Ghép dấu chấm và phần thập phân
    *str++ = '.';
    *str++ = dec_part + '0';
    *str = '\0';
}