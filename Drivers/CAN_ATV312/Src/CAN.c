#include "CAN.h"
#include "timer.h"

volatile CAN_RxFrame_t g_rx_frame;


// Tang low
void CAN_Init(void)
{
    /**************GPIO configuration***********/
    RCC->AHB1ENR   |= (GPIOD_EN);
    RCC->APB1ENR   |= (CAN1_EN);

    // Set GPIOD PD0 (RX), PD1 (TX)
    GPIOD->MODER   &= ~(3U << 0);
    GPIOD->MODER   |=  (CAN_RX_PIN);
    GPIOD->MODER   &= ~(3U << 2);
    GPIOD->MODER   |=  (CAN_TX_PIN);

    GPIOD->AFR[0] &= ~(0xFU << 0);
    GPIOD->AFR[0] &= ~(0xFU << 4);
    GPIOD->AFR[0] |=  (9U << 0);
    GPIOD->AFR[0] |=  (9U << 4);    

    GPIOD->OSPEEDR |= (3U << 0);
    GPIOD->OSPEEDR |= (3U << 2);

    GPIOD->PUPDR   &= ~(3U << 0);
    GPIOD->PUPDR   |=  (1U << 0);

    /*********CAN1 Configuration********/
    CAN1->MCR |= (CAN_MCR_INRQ);
    CAN1->MCR &= ~(CAN_MCR_SLEEP);
    while (!(CAN1->MSR & CAN_MSR_INAK));

    CAN1->MCR |= (CAN_MCR_ABOM);
    
    // Baudrate 250 kbps
    CAN1->BTR = (0U  << 24) | 
                (1U  << 20) | 
                (10U << 16) | 
                (11U << 0);   

    CAN1->MCR &= ~(CAN_MCR_INRQ);
    while (CAN1->MSR & CAN_MSR_INAK);

    /*****************Bộ lọc Filter 0************/
    CAN1->FMR   |=  (CAN_FMR_FINIT); 
    CAN1->FA1R  &= ~(CAN_FA1R_FACT0);
    CAN1->FS1R  |=  (CAN_FS1R_FSC0); 
    CAN1->FM1R  &= ~(CAN_FM1R_FBM0); 
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R &= ~(CAN_FFA1R_FFA0); // FIFO 0
    CAN1->FA1R  |=  (CAN_FA1R_FACT0);
    CAN1->FMR   &= ~(CAN_FMR_FINIT);

    /*************Cấu hình ngắt CAN**************/
    // Chỉ bật khi hàm IRQHandler bên dưới được định nghĩa!
    CAN1->IER |= (CAN_IER_FMPIE0);
    NVIC_SetPriority(CAN1_RX0_IRQn, 1);
    NVIC_EnableIRQ(CAN1_RX0_IRQn);     
}

uint8_t CAN_Send_Frame(uint16_t stid, uint8_t *data, uint8_t dlc)
{
    // Tìm mailbox rảnh chính xác
    uint8_t mbx;
    if (CAN1->TSR & CAN_TSR_TME0)      mbx = 0;
    else if (CAN1->TSR & CAN_TSR_TME1) mbx = 1;
    else if (CAN1->TSR & CAN_TSR_TME2) mbx = 2;
    else return 0; // Hết mailbox trống

    CAN1->sTxMailBox[mbx].TIR  = ((uint32_t)stid << 21);
    CAN1->sTxMailBox[mbx].TDTR = (dlc & 0x0F);
    
    uint8_t payload[8] = {0};
    for (uint8_t i = 0; i < dlc && i < 8; i++)
    {
        payload[i] = data[i];
    }
    CAN1->sTxMailBox[mbx].TDLR = ((uint32_t)payload[3] << 24) |
                                 ((uint32_t)payload[2] << 16) |
                                 ((uint32_t)payload[1] << 8)  |
                                 ((uint32_t)payload[0]);

    CAN1->sTxMailBox[mbx].TDHR = ((uint32_t)payload[7] << 24) |
                                 ((uint32_t)payload[6] << 16) |
                                 ((uint32_t)payload[5] << 8)  |
                                 ((uint32_t)payload[4]);

    CAN1->sTxMailBox[mbx].TIR |= CAN_TI0R_TXRQ;
    return 1;
}

void CAN1_RX0_IRQHandler(void)
{
    if ((CAN1->RF0R & CAN_RF0R_FMP0) != 0)
    {
        g_rx_frame.id          = (CAN1->sFIFOMailBox[0].RIR >> 21) & 0x7FF;
        g_rx_frame.dlc         = (CAN1->sFIFOMailBox[0].RDTR & 0x0F);

        uint32_t low_reg       = CAN1->sFIFOMailBox[0].RDLR;
        uint32_t high_reg      = CAN1->sFIFOMailBox[0].RDHR;

        g_rx_frame.rx_data[0] = (uint8_t)(low_reg >> 0);
        g_rx_frame.rx_data[1] = (uint8_t)(low_reg >> 8);
        g_rx_frame.rx_data[2] = (uint8_t)(low_reg >> 16);
        g_rx_frame.rx_data[3] = (uint8_t)(low_reg >> 24);

        g_rx_frame.rx_data[4] = (uint8_t)(high_reg >> 0);
        g_rx_frame.rx_data[5] = (uint8_t)(high_reg >> 8);
        g_rx_frame.rx_data[6] = (uint8_t)(high_reg >> 16);
        g_rx_frame.rx_data[7] = (uint8_t)(high_reg >> 24);

        g_rx_frame.is_new_data = 1;

        CAN1->RF0R |= CAN_RF0R_RFOM0; 
    }
}

// ATV312

int8_t CANOpen_SDO_Write16(uint8_t node_id, uint16_t index, uint8_t sub_index, int16_t value)
{
    // ham nay dung de ghi 16 bit nen dung 2 byte {4; 5} la du
    uint8_t payload[8] = {0};

    payload[0] = SDO_W2byte;
    payload[1] = (uint8_t)(index & 0xFF);
    payload[2] = (uint8_t)((index >>8) & 0xFF);
    payload[3] = sub_index;
    uint16_t u_value = (uint16_t)value;
    payload[4] = (uint8_t)(value & 0xFF);
    payload[5] = (uint8_t)((value >> 8) & 0xFF);

    g_rx_frame.is_new_data = 0;

    // phat frame do toi dia chi.
    if(!CAN_Send_Frame(SDO_COBID_RQ + node_id, payload, 8))
    {
        return 0; // neu truyen that bai
    }

    // Cho bien tan phan hoi tu dia chi SDO_COBID_RP
    uint32_t timeout = 500;
    while (timeout > 0)
    {
        if (g_rx_frame.is_new_data)
        {
            g_rx_frame.is_new_data = 0;
            // kiem tra co phai phan hoi tu bien tan dia chi node_id khong
            if (g_rx_frame.id == (SDO_COBID_RP + node_id))
            {
                // Bien tan cho phep va da thanh cong
                if (g_rx_frame.rx_data[0] == SDO_W124byte_RP_Scc) return 1;
                // Bien tan tu choi
                if (g_rx_frame.rx_data[0] == SDO_RP_Err) return -1;
            }
        }
        Delay_ms_TIM2(1);
        timeout--;
    }
    return 0; // het thoi gian khong nhan dc phan hoi
}

int8_t CANOpen_SDO_Read16(uint8_t node_id, uint16_t index, uint8_t sub_index, uint16_t *ovalue)
{
    uint8_t payload[8] = {0};

    payload[0] = SDO_Rdata;
    payload[1] = (uint8_t)(index & 0xFF);
    payload[2] = (uint8_t)((index >> 8) & 0xFF);
    payload[3] = sub_index;

    g_rx_frame.is_new_data = 0;
    
    if (!CAN_Send_Frame(SDO_COBID_RQ + node_id, payload, 8))
    {
        return 0;
    }
    uint32_t timeout = 500;
    while (timeout > 0)
    {
        if (g_rx_frame.is_new_data)
        {
            g_rx_frame.is_new_data = 0;
            if (g_rx_frame.id == (SDO_COBID_RP + node_id))
            {
                if (g_rx_frame.rx_data[0] ==  SDO_R2byte_RP_Scc || g_rx_frame.rx_data[0] == SDO_R4byte_RP_Scc)
                {
                    if (ovalue != 0)
                    {
                        *ovalue = (uint16_t)(g_rx_frame.rx_data[4]) | ((uint16_t)g_rx_frame.rx_data[5] << 8);
                    }
                    return 1;
                }
                if (g_rx_frame.rx_data[0] == SDO_RP_Err)
                {
                    return -1;
                }
            }
        }
        Delay_ms_TIM2(1);
        timeout--;
    }
    return 0;
}

uint8_t ATV312_SDO_Power_Init(uint8_t node_id)
{
    // Control of the NMT state machine.
    // Client -> Drive
    uint8_t nmt_payload[2] = {NMT_Start_Remote, node_id};
    CAN_Send_Frame(NMT_ID, nmt_payload, 2);
    Delay_ms_TIM2(100);

    // Go trang thai dung an toan
    CANOpen_SDO_Write16(node_id, CMDD, 0x00, 0x0000);
    Delay_ms_TIM2(100);
    CANOpen_SDO_Write16(node_id, CMDD, 0x00, 0x0080);
    Delay_ms_TIM2(50);
    CANOpen_SDO_Write16(node_id, CMDD, 0x00, 0x0000);
    Delay_ms_TIM2(50);

    // Shutdown -> rDy
    CANOpen_SDO_Write16(node_id, CMDD, 0x00, CMDD_RDY);
    Delay_ms_TIM2(150);

    // Tien cong suat
    CANOpen_SDO_Write16(node_id, CMDD, 0x00, CMDD_SwOn);
    Delay_ms_TIM2(150);
    
    // Vao che do operational (bat IGBT)
    CANOpen_SDO_Write16(node_id, CMDD, 0x00, CMDD_EnOpe);
    Delay_ms_TIM2(150);
    return 1;
}

// 1. Ghi tốc độ đặt theo Hz (-60.0Hz đến +60.0Hz)
int8_t ATV312_SDO_Set_Speed_Hz(uint8_t node_id, float hz)
{
    if (hz > 60.0f)  hz = 60.0f;
    if (hz < -60.0f) hz = -60.0f;

    int16_t rpm = (int16_t)(hz * 30.0f);
    return CANOpen_SDO_Write16(node_id, LFRD, 0x00, (uint16_t)rpm);
}

// 2. Đọc tốc độ thực tế động cơ đang quay (quy đổi ra Hz)
int8_t ATV312_SDO_Get_Speed_Hz(uint8_t node_id, float *actual_hz)
{
    uint16_t raw_val = 0;
    // Đọc từ thanh ghi 0x6044 (RFRD)
    int8_t status = CANOpen_SDO_Read16(node_id, RFRD, 0x00, &raw_val);

    if (status == 1 && actual_hz != 0)
    {
        int16_t rpm = (int16_t)raw_val; // Ép về số có dấu bù 2
        *actual_hz = (float)rpm / 30.0f;
        return 1;
    }
    return status;
}

int8_t ATV312_SDO_Get_Current_A(uint8_t node_id, float *current_a)
{
    uint16_t raw_val = 0;
    int8_t status = CANOpen_SDO_Read16(node_id, LCr, LCr_sub_inde, &raw_val); //

    if (status == 1 && current_a != 0)
    {
        *current_a = (float)raw_val / 10.0f; // don vi 0.1A nen chia 10f
        return 1;
    }

    return status;
}


int8_t ATV312_SDO_Run(uint8_t node_id, Motor_Dir_t dir)
{
    uint16_t cmd = (dir == DIR_REVERSE) ? 0x080F : 0x000F;
    return CANOpen_SDO_Write16(node_id, 0x6040, 0x00, cmd);
}
