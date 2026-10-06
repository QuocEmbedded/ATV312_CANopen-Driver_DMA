#include "UART_DMA.h"
#include "stm32f407xx.h"

/******************** DEFINITIONS ********************/
/* Chỉ giữ lại các macro mà stm32f407xx.h chưa định nghĩa tiện lợi */
#define RCC_AHB1_DMA2_EN         (1U << 22)

#define DMA_SCR_CH4_SELECT       (4U << 25)
#define DMA_SCR_MINC             (1U << 10)
#define DMA_SCR_CIRC             (1U << 8)
#define DMA_SCR_DIR_M2P          (1U << 6)
#define DMA_SCR_TCIE             (1U << 4)
#define DMA_SCR_EN               (1U << 0)

/* Mask xóa cờ Stream 5 và Stream 7 trên HIFCR */
#define DMA_HIFCR_STREAM5_CLEAR  (0x3DU << 6)
#define DMA_HIFCR_STREAM7_CLEAR  (0x3DU << 22)

/******************** PRIVATE VARIABLES ********************/
static char dma_rx_buffer[UART_DATA_BUFF_SIZE];
static uint8_t app_rx_buffer[UART_DATA_BUFF_SIZE];

static volatile uint8_t  s_rx_ready = 0;
static volatile uint16_t s_rx_len   = 0;
static volatile uint8_t  s_tx_uart_cmplt = 0;

static uint16_t last_pos = 0;

/******************** FUNCTIONS ********************/
void UART1_GPIO_Init(void)
{
    // 1. Cấu hình clock
    RCC->AHB1ENR |= (1U << 0); // Bật clock cho GPIOA
    RCC->APB2ENR |= (1U << 4); // Bật clock cho USART1

    // 2. Cấu hình chân GPIO (PA9 - TX, PA10 - RX)
    GPIOA->MODER &= ~(3U << 18);
    GPIOA->MODER |=  (2U << 18); // Alternate Function PA9
    GPIOA->MODER &= ~(3U << 20);
    GPIOA->MODER |=  (2U << 20); // Alternate Function PA10  
    
    GPIOA->OTYPER &= ~(1U << 9);  // Push-pull PA9
    GPIOA->OTYPER &= ~(1U << 10); // Push-pull PA10

    GPIOA->OSPEEDR &= ~(3U << 18);
    GPIOA->OSPEEDR |=  (2U << 18); // High speed PA9
    GPIOA->OSPEEDR &= ~(3U << 20);
    GPIOA->OSPEEDR |=  (2U << 20); // High speed PA10

    GPIOA->AFR[1] &= ~(0xFU << 4);
    GPIOA->AFR[1] |=  (7U << 4);   // AF7 cho PA9
    GPIOA->AFR[1] &= ~(0xFU << 8);
    GPIOA->AFR[1] |=  (7U << 8);   // AF7 cho PA10

    // 3. Cấu hình USART1 Controller
    USART1->CR1 &= ~(1U << 13); // Tắt UE trước khi cấu hình
    USART1->BRR = 0x2D9;        // Baudrate 115200 @ 84MHz APB2
    USART1->CR1 &= ~(1U << 12); // 1 start bit, 8 data bits, n word length

    /* ĐÃ BỎ: Dòng bật ngắt RXNEIE (1U << 5) vì đã dùng DMA nhận dữ liệu ngầm */

    USART1->CR1 |= (3U << 2);   // Bật TE và RE
    USART1->CR1 |= (1U << 13);  // Bật UE
}

void UART1_DMA_Init(void)
{
    /* 1. Bật clock DMA2 */
    RCC->AHB1ENR |= RCC_AHB1_DMA2_EN;

    /***** 2. RX: DMA2 Stream 5 Channel 4 *****/
    DMA2_Stream5->CR &= ~DMA_SCR_EN;
    while (DMA2_Stream5->CR & DMA_SCR_EN);

    DMA2->HIFCR = DMA_HIFCR_STREAM5_CLEAR;

    DMA2_Stream5->PAR  = (uint32_t)&(USART1->DR);
    DMA2_Stream5->M0AR = (uint32_t)dma_rx_buffer;
    DMA2_Stream5->NDTR = UART_DATA_BUFF_SIZE;

    /* Kênh 4 | Tự tăng RAM | Chế độ Circular (TẮT ngắt TCIE để DMA tự chạy ngầm) */
    DMA2_Stream5->CR = DMA_SCR_CH4_SELECT | DMA_SCR_MINC | DMA_SCR_CIRC;
    DMA2_Stream5->CR |= DMA_SCR_EN;

    /***** 3. TX: DMA2 Stream 7 Channel 4 *****/
    DMA2_Stream7->CR &= ~DMA_SCR_EN;
    while (DMA2_Stream7->CR & DMA_SCR_EN);

    DMA2->HIFCR = DMA_HIFCR_STREAM7_CLEAR;
    DMA2_Stream7->PAR = (uint32_t)&(USART1->DR);
    DMA2_Stream7->CR  = DMA_SCR_CH4_SELECT | DMA_SCR_DIR_M2P | DMA_SCR_MINC | DMA_SCR_TCIE;

    /***** 4. Cấu hình USART1 DMA Requests & Interrupt *****/
    USART1->CR3 |= USART_CR3_DMAT | USART_CR3_DMAR;
    USART1->CR1 |= USART_CR1_IDLEIE; // Bật ngắt IDLE khi đường truyền rảnh
    USART1->SR  &= ~USART_SR_TC;

    /***** 5. Cấu hình NVIC *****/
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);

    /* Stream 5 RX chạy Circular tự động nên không cần bật ngắt NVIC */
    NVIC_SetPriority(DMA2_Stream7_IRQn, 1);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

void UART1_SendString_DMA(const char *str, uint16_t len)
{
    while (DMA2_Stream7->CR & DMA_SCR_EN);

    DMA2->HIFCR = DMA_HIFCR_STREAM7_CLEAR;
    USART1->SR &= ~USART_SR_TC;
    s_tx_uart_cmplt = 0;

    DMA2_Stream7->M0AR = (uint32_t)str;
    DMA2_Stream7->NDTR = len;
    DMA2_Stream7->CR  |= DMA_SCR_EN;
}

uint16_t UART1_ReadPacket(uint8_t *dest, uint16_t max_len)
{
    if (!s_rx_ready)
    {
        return 0;
    }

    uint16_t len = (s_rx_len < max_len) ? s_rx_len : max_len;
    for (uint16_t i = 0; i < len; i++)
    {
        dest[i] = app_rx_buffer[i];
    }

    s_rx_ready = 0; // Đã đọc xong, xóa cờ
    return len;
}

/******************** ISRs ********************/

void DMA2_Stream7_IRQHandler(void)
{
    if (DMA2->HISR & DMA_HISR_TCIF7)
    {
        DMA2->HIFCR = (1U << 27); // Xóa cờ CTCIF7
        s_tx_uart_cmplt = 1;
    }
}

/* ĐÃ XÓA: Hàm DMA2_Stream5_IRQHandler vì không còn gán last_pos = 0 làm rách gói tin */

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;

    /* A. Sự kiện IDLE: Nhận gói tin biến thiên độ dài */
    if (sr & USART_SR_IDLE)
    {
        /* 1. Trình tự bắt buộc xóa cờ IDLE: Đọc SR rồi đọc DR */
        volatile uint32_t dummy = USART1->SR;
        dummy = USART1->DR;
        (void)dummy;

        /* 2. Vị trí hiện tại của DMA trong bộ đệm */
        uint16_t current_pos = UART_DATA_BUFF_SIZE - (uint16_t)(DMA2_Stream5->NDTR);

        /* 3. Xử lý khi có dữ liệu mới */
        if (current_pos != last_pos)
        {
            uint16_t len = 0;

            if (current_pos > last_pos)
            {
                /* Gói tin nằm xuôi liên tục */
                len = current_pos - last_pos;
                for (uint16_t i = 0; i < len; i++)
                {
                    app_rx_buffer[i] = (uint8_t)dma_rx_buffer[last_pos + i];
                }
            }
            else
            {
                /* Gói tin bị cuộn tròn qua mép cuối buffer (Wrap-around) */
                len = (UART_DATA_BUFF_SIZE - last_pos) + current_pos;
                uint16_t idx = 0;

                /* Chép đoạn cuối mảng */
                for (uint16_t i = last_pos; i < UART_DATA_BUFF_SIZE; i++)
                {
                    app_rx_buffer[idx++] = (uint8_t)dma_rx_buffer[i];
                }
                /* Chép tiếp đoạn đầu mảng */
                for (uint16_t i = 0; i < current_pos; i++)
                {
                    app_rx_buffer[idx++] = (uint8_t)dma_rx_buffer[i];
                }
            }

            s_rx_len = len;
            last_pos = current_pos; // Luôn cập nhật vị trí mới
            s_rx_ready = 1;         // Báo cho main có gói mới
        }
    }
}