#include <stdint.h>
#include <stddef.h>
#include "font.h"

// ==========================================
// 硬體 I/O 巨集
// ==========================================
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ==========================================
// Multiboot 結構 (擷取圖形模式相關資訊)
// ==========================================
#define MULTIBOOT_MAGIC 0x2BADB002

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    // Framebuffer 資訊 (於 Multiboot offset 88)
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} __attribute__((packed)) multiboot_info_t;

// ==========================================
// 圖形全域變數
// ==========================================
static uint32_t* fb_addr = NULL;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;

// ==========================================
// 繪圖基本函式
// ==========================================
void putpixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;
    fb_addr[y * (fb_pitch / 4) + x] = color;
}

void fill_screen(uint32_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            putpixel(x, y, color);
        }
    }
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = x; i < x + w; i++) {
        putpixel(i, y, color);
        putpixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        putpixel(x, j, color);
        putpixel(x + w - 1, j, color);
    }
}

void fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            putpixel(i, j, color);
        }
    }
}

// ==========================================
// 文字渲染
// ==========================================
void draw_char(int x, int y, char c, uint32_t color, int scale) {
    if (c < 32 || c > 90) c = '?'; // Fallback to '?' if out of range, or ignore
    if (c == '?') c = 63;
    
    int index = c - 32;
    for (int row = 0; row < 8; row++) {
        uint8_t row_data = font8x8[index][row];
        for (int col = 0; col < 8; col++) {
            if ((row_data >> (7 - col)) & 1) {
                // Scale factor
                for(int dy = 0; dy < scale; dy++) {
                    for(int dx = 0; dx < scale; dx++) {
                        putpixel(x + col * scale + dx, y + row * scale + dy, color);
                    }
                }
            }
        }
    }
}

void draw_string(int x, int y, const char* str, uint32_t color, int scale) {
    int cur_x = x;
    while (*str) {
        draw_char(cur_x, y, *str, color, scale);
        cur_x += 8 * scale + (2 * scale); // 加上字距
        str++;
    }
}

// ==========================================
// 休眠函數 (粗略的 Delay)
// ==========================================
void delay(volatile uint32_t count) {
    while (count--) {
        asm volatile ("pause");
    }
}

// ==========================================
// Cyberpunk UI 動畫與繪製
// ==========================================
#define COLOR_BG      0x050510  // 深邃夜空藍
#define COLOR_NEON_B  0x00D9FF  // 賽博青色
#define COLOR_NEON_P  0xB026FF  // 霓虹紫色
#define COLOR_ALERT   0xFF003C  // 警告紅

void boot_animation() {
    fill_screen(COLOR_BG);
    
    // 繪製霓虹邊框 (兩層做出發光感)
    draw_rect(20, 20, fb_width - 40, fb_height - 40, COLOR_NEON_P);
    draw_rect(22, 22, fb_width - 44, fb_height - 44, COLOR_NEON_B);

    int text_x = fb_width / 2 - 140;
    int text_y = fb_height / 2 - 50;
    
    draw_string(text_x, text_y, "LOADING SYSTEM...", COLOR_NEON_B, 2);

    // 進度條邊框
    int bar_w = 400;
    int bar_h = 20;
    int bar_x = (fb_width - bar_w) / 2;
    int bar_y = fb_height / 2 + 20;
    draw_rect(bar_x, bar_y, bar_w, bar_h, COLOR_NEON_P);

    // 模擬載入動畫
    for (int i = 0; i < bar_w - 4; i += 8) {
        fill_rect(bar_x + 2 + i, bar_y + 2, 6, bar_h - 4, COLOR_NEON_B);
        delay(3000000); // 延遲，實際執行速度視硬體/VM而定
    }

    delay(20000000);
}

void render_desktop() {
    fill_screen(COLOR_BG);

    // 外框與裝飾
    draw_rect(10, 10, fb_width - 20, fb_height - 20, COLOR_NEON_B);
    draw_rect(15, 15, fb_width - 30, fb_height - 30, COLOR_NEON_P);
    
    // 頂部標題列
    fill_rect(15, 15, fb_width - 30, 30, COLOR_NEON_P);
    draw_string(25, 20, "NEON TERMINAL V1.0", COLOR_BG, 2);

    // 中央大字
    draw_string(fb_width / 2 - 160, fb_height / 2 - 40, "MYCOOLOS READY", COLOR_NEON_B, 3);
    draw_string(fb_width / 2 - 120, fb_height / 2 + 30, "PRESS ANY KEY TO TYPE", COLOR_NEON_P, 1);

    // 底部狀態列
    fill_rect(15, fb_height - 40, fb_width - 30, 25, COLOR_NEON_B);
    draw_string(25, fb_height - 35, "STATUS: ONLINE | MEM: 640K SECURE | SYS: OK", COLOR_BG, 1);
}

// ==========================================
// 簡易鍵盤輸入 (Polling 方式)
// ==========================================
void run_input_loop() {
    int cursor_x = 30;
    int cursor_y = fb_height / 2 + 70;
    draw_string(cursor_x, cursor_y, ">", COLOR_NEON_B, 2);
    cursor_x += 24;

    uint8_t last_scancode = 0;

    while (1) {
        // 檢查鍵盤狀態暫存器 (Port 0x64)，看 buffer 是否有資料 (bit 0 = 1)
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            
            // 避免重複處理解放鍵 (Release key) 或者連續按鍵
            if (scancode < 0x80 && scancode != last_scancode) {
                // 這裡僅示範：任意鍵按下時繪製一個方塊模擬輸入
                fill_rect(cursor_x, cursor_y, 14, 16, COLOR_NEON_P);
                cursor_x += 20;
                
                // 換行邏輯
                if (cursor_x > (int)fb_width - 50) {
                    cursor_x = 54;
                    cursor_y += 30;
                }
            }
            last_scancode = scancode;
        }
    }
}

// ==========================================
// Kernel 進入點
// ==========================================
void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
    // 檢查 Multiboot 魔術數字
    if (magic != MULTIBOOT_MAGIC) {
        return;
    }

    // 檢查 Framebuffer 是否有效 (flags bit 12 須為 1)
    if (mbi->flags & (1 << 12)) {
        fb_addr = (uint32_t*)(uint32_t)(mbi->framebuffer_addr);
        fb_width = mbi->framebuffer_width;
        fb_height = mbi->framebuffer_height;
        fb_pitch = mbi->framebuffer_pitch;
        
        // 執行炫酷科幻啟動介面
        boot_animation();
        
        // 渲染主畫面
        render_desktop();

        // 進入簡易的鍵盤監聽迴圈 (骨架)
        run_input_loop();
    }

    // 防禦性停機 (不應該執行到這裡)
    while (1) {
        asm volatile ("hlt");
    }
}
