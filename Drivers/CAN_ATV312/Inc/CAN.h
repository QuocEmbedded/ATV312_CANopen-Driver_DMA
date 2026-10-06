#ifndef CAN_H_
#define CAN_H_
#include "stm32f407xx.h"


#define GPIOD_EN           (1U<<3)  // Bat GPIOD
#define CAN1_EN            (1U<<25) // Bat CAN1 
#define CAN_RX_PIN         (2U<<0) // PD0
#define CAN_TX_PIN         (2U<<2) // PD1

#define ATV312_NODE_ID    2U
#define NMT_ID            0x00U

/***************** IEC 61800 - 7 Variable*************/
/*Index*/ 
#define CMDD               0x6040U
#define LFRD               0x6042U
#define RFRD               0x6044U // output speed

/*Value by index*/
#define CMDD_RDY           0x0006U
#define CMDD_SwOn          0x0007U
#define CMDD_EnOpe         0x000FU

/******************Monitoring variable***************/
#define LCr                0x2002U // Current in the motor
#define LCr_sub_inde       0x05U   // sub_index of current in the motor


/* */
#define SDO_COBID_RQ        0x600U   // mat ma dinh danh COB-ID cua phia truyen   (client -> drive)
#define SDO_COBID_RP        0x580U   // mat ma dinh danh COB-ID cua phia phan hoi (client <- drive)

#define SDO_W4byte          0x23U    // byte[0]. Cilent yeu cau truyen 4 byte
#define SDO_W2byte          0x2BU    // byte[0]. Cilent yeu cau truyen 2 byte
#define SDO_Rdata           0x40U    // byte[0]. Cilent yeu cau doc data

#define SDO_W124byte_RP_Scc 0x60U    // byte[0]. Drive bao write 1/2/4 byte thanh cong
#define SDO_R2byte_RP_Scc   0x4BU    // byte[0]. Drive bao read 2 byte thanh cong
#define SDO_R4byte_RP_Scc   0x43U    // byte[0]. Drive bao read 4 byte thanh cong
#define SDO_RP_Err          0x80U    // byte[0]. Drive bao loi

/* */
#define NMT_Start_Remote    0x01U    // Start_Remote_Node
#define NMT_Stop_Remote     0x02U    // Stop_Remote_Node
#define Enter_Pre_Ope       0x80U    // Enter_Pre-Operational_State
#define NMT_Reset_Node      0x81U    // Enter_Pre-Operational_State
#define NMT_Reset_Com       0x82U    // Reset_Communication

typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t rx_data[8];
    volatile uint8_t is_new_data;
} CAN_RxFrame_t;

typedef enum {
    DIR_FORWARD = 0,
    DIR_REVERSE = 1
} Motor_Dir_t;


void CAN_Init(void);
uint8_t CAN_Send_Frame(uint16_t stid, uint8_t *data, uint8_t dlc);
int8_t CANOpen_SDO_Write16(uint8_t node_id, uint16_t index, uint8_t sub_index, int16_t value);
int8_t CANOpen_SDO_Read16(uint8_t node_id, uint16_t index, uint8_t sub_index, uint16_t *ovalue);
uint8_t ATV312_SDO_Power_Init(uint8_t node_id);
int8_t ATV312_SDO_Set_Speed_Hz(uint8_t node_id, float hz);
int8_t ATV312_SDO_Get_Speed_Hz(uint8_t node_id, float *actual_hz);
int8_t ATV312_SDO_Get_Current_A(uint8_t node_id, float *current_a);
int8_t ATV312_SDO_Run(uint8_t node_id, Motor_Dir_t dir);
#endif
