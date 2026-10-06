/* Định nghĩa Entry Point của chương trình (Hàm reset handler) */
ENTRY(Reset_Handler)

/* Định nghĩa vùng nhớ của STM32F407VET6 */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Định nghĩa đỉnh Stack (Nằm ở cuối vùng nhớ SRAM) */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    /* Vector Table phải nằm đầu tiên trong vùng nhớ FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code chương trình và dữ liệu hằng số (Read-Only) */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Lưu vị trí bắt đầu của .data trong FLASH để Startup sao chép sang SRAM */
    _sidata = LOADADDR(.data);

    /* Dữ liệu đã khởi tạo (Read-Write) sẽ chạy trên SRAM */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* Địa chỉ xuất phát của .data trong SRAM */
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;        /* Địa chỉ kết thúc của .data trong SRAM */
    } > SRAM AT > FLASH

    /* Dữ liệu chưa khởi tạo (Read-Write), tự động xóa về 0 khi boot */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;        /* Địa chỉ xuất phát của .bss trong SRAM */
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;        /* Địa chỉ kết thúc của .bss trong SRAM */
    } > SRAM

    /* Kiểm tra xem dung lượng Stack có vượt quá giới hạn không (Tùy chọn) */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
