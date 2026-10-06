# =============================================================================
# 1. TÊN DỰ ÁN & THƯ MỤC
# =============================================================================
TARGET = firmware
BUILD_DIR = build

# =============================================================================
# 2. CÔNG CỤ BIÊN DỊCH
# =============================================================================
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# =============================================================================
# 3. TÌM KIẾM FILE NGUỒN VÀ FILE HEADER
# =============================================================================
# Quét tất cả file .c trong Core/Src và Drivers
C_SOURCES = \
$(wildcard Core/Src/*.c) \
$(wildcard Drivers/Bsp/Src/*.c) \
$(wildcard Drivers/CAN_ATV312/Src/*.c)

# Tên file Assembly khởi động
ASM_SOURCES = Startup/startup_stm32f407xx.s
# Các thư mục chứa file .h
C_INCLUDES = \
-ICore/Inc \
-IDrivers/Bsp/Inc \
-IDrivers/CAN_ATV312/Inc \
-IDrivers/Cmsis/Include \
-IDrivers/Cmsis/Devices/ST/STM32F4xx/Include
# =============================================================================
# 4. CẤU HÌNH CỜ BIÊN DỊCH (FLAGS)
# =============================================================================
# Thông số lõi ARM Cortex-M4 của STM32F4
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=softfp
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Các macro định nghĩa
C_DEFS = \
-DSTM32F407xx \
-DHSE_VALUE=8000000UL \
-DHSI_VALUE=16000000UL

# Cờ biên dịch C (Bật cảnh báo, tắt tối ưu hóa -O0 để dễ debug)
# -fdata-sections -ffunction-sections giúp Linker xóa các hàm không dùng đến
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -O0 -Wall -fdata-sections -ffunction-sections

# File bản đồ bộ nhớ (Linker script)
LDSCRIPT = linker.ld

# Cờ liên kết (Sử dụng picolibc thay cho chuẩn C thông thường để nhẹ hơn)
# -Wl,--gc-sections dọn rác các hàm mồ côi
LDFLAGS = $(MCU) -specs=picolibc.specs -T$(LDSCRIPT) -nostartfiles -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# =============================================================================
# 5. QUẢN LÝ ĐƯỜNG DẪN BUILD VÀ TẠO OBJECTS
# =============================================================================
# Lấy tên các file .c, đổi đuôi thành .o và thêm tiền tố "build/" ở trước
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
# Hướng dẫn Make tìm các file .c ở trong các thư mục gốc của nó
vpath %.c $(sort $(dir $(C_SOURCES)))

# Tương tự với file Assembly
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# =============================================================================
# 6. QUY TRÌNH BIÊN DỊCH (TARGETS)
# =============================================================================
# Chạy mặc định khi gõ `make`
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# Biên dịch từng file .c thành .o (Cần thư mục build tồn tại trước)
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	@echo "[CC] $<"
	@$(CC) -c $(CFLAGS) $< -o $@

# Biên dịch file .s thành .o
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "[AS] $<"
	@$(AS) -c $(CFLAGS) $< -o $@

# Link tất cả file .o thành file .elf
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "[LINK] $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "--- Kich thuoc Firmware ---"
	@$(SZ) $@

# Tạo file .hex
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "[HEX] $@"
	@$(HEX) $< $@
	
# Tạo file .bin
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "[BIN] $@"
	@$(BIN) $< $@

# Tạo thư mục build nếu nó chưa tồn tại
$(BUILD_DIR):
	mkdir $@

# =============================================================================
# 7. CÁC LỆNH TIỆN ÍCH (CLEAN, FLASH)
# =============================================================================
clean:
	@echo "[CLEAN] Xoa thu muc $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)

flash: all
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

.PHONY: all clean flash