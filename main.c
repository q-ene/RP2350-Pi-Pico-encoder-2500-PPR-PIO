#include <stdio.h>
#include "pico/stdlib.h"
#include "encoder.h"
#include "lcd_i2c.h"

static void format_um(char *buf, int32_t um) {
    bool neg = um < 0;
    uint32_t v = neg ? (uint32_t)(-(int64_t)um) : (uint32_t)um;
    uint32_t mm = v / 1000;
    uint32_t frac = v % 1000;
    snprintf(buf, 10, "%c%05lu.%03lu",
             neg ? '-' : ' ', (unsigned long)mm, (unsigned long)frac);
}

int main(void) {
    stdio_init_all();
    lcd_init();

    lcd_set_cursor(0, 0);
    lcd_string("Pos X:  ---.---");
    lcd_set_cursor(0, 1);
    lcd_string("Z: 0      ---");

    encoder_init();

    char buf[16];
    int32_t last_um = -1;
    int32_t last_z = -1;
    int8_t last_dir = 127;
    uint32_t last_diag = 0;

    while (true) {
        if (!encoder_home_seen()) {
            last_um = -1;
        } else {
            int32_t um = encoder_get_um();
            if (um != last_um) {
                format_um(buf, um);
                lcd_set_cursor(7, 0);
                lcd_string(buf);
                last_um = um;
            }
        }

        int32_t z = encoder_z_count();
        if (z != last_z) {
            snprintf(buf, sizeof buf, "Z: %-6ld", (long)z);
            lcd_set_cursor(0, 1);
            lcd_string(buf);
            last_z = z;
        }

        int8_t dir = encoder_direction();
        if (encoder_home_seen() && dir != last_dir) {
            lcd_set_cursor(10, 1);
            lcd_string(dir > 0 ? ">>>" : (dir < 0 ? "<<<" : "---"));
            last_dir = dir;
        }

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_diag > 500) {
            printf("cnt=%ld um=%ld z=%ld dir=%d\n",
                   (long)encoder_get_counts(), (long)encoder_get_um(),
                   (long)encoder_z_count(), encoder_direction());
            last_diag = now;
        }

        sleep_ms(10);
    }
}
