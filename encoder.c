#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "encoder.h"
#include "encoder.pio.h"

#define Z_MIN_GAP_US 1000   // минимальный интервал между фронтами Z, мкс
#define DIR_DEADBAND 4      // зона нечувствительности для направления, отсчёты

static PIO pio = pio0;
static uint sm;
static uint dma_ch;

static volatile uint32_t raw_count;
static volatile int32_t  bias;
static volatile bool     z_seen;
static volatile int32_t  z_count;

static int32_t last_z_cnt;
static absolute_time_t last_t;
static int32_t last_dir_cnt;

static void encoder_z_irq(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_RISE)) return;

    // Антидребезг по времени: таймер обновляем на КАЖДОМ фронте,
    // иначе дребезг после короткой паузы проходит как новый фронт.
    absolute_time_t t = get_absolute_time();
    if (absolute_time_diff_us(last_t, t) < Z_MIN_GAP_US) {
        last_t = t;
        return;
    }
    last_t = t;

    int32_t cnt_now = (int32_t)raw_count;

    if (!z_seen) {
        bias = cnt_now;          // первая метка — начало отсчёта
        z_seen = true;
        last_z_cnt = cnt_now;
        z_count++;
        return;
    }

    // Квадратурный фильтр: метка принимается, только если с предыдущей
    // принятой вал прошёл хотя бы пол-оборота. Дребезг живёт в единицах
    // отсчётов и сюда не проходит никогда.
    int32_t d = cnt_now - last_z_cnt;
    if (d < 0) d = -d;
    if (d < ENC_COUNTS_PER_REV / 2) return;

    last_z_cnt = cnt_now;
    z_count++;
}

void encoder_init(void) {
    // --- PIO: квадратурный декодер ---
    uint offset = pio_add_program(pio, &quadrature_encoder_program);
    sm = pio_claim_unused_sm(pio, true);
    quadrature_encoder_program_init(pio, sm, offset, ENC_A_PIN, 0);

    // --- DMA: непрерывный слив последнего значения счётчика из RX FIFO ---
    dma_ch = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, sm, false));
    channel_config_set_chain_to(&cfg, dma_ch);   // самоперезапуск
    dma_channel_configure(dma_ch, &cfg, (void *)&raw_count, &pio->rxf[sm], 1, true);

    // --- Z: референсная метка ---
    gpio_init(ENC_Z_PIN);
    gpio_set_dir(ENC_Z_PIN, GPIO_IN);
    gpio_pull_up(ENC_Z_PIN);
    gpio_set_irq_enabled_with_callback(ENC_Z_PIN, GPIO_IRQ_EDGE_RISE, true, encoder_z_irq);
}

int32_t encoder_get_counts(void) {
    return (int32_t)raw_count;
}

bool encoder_home_seen(void) {
    return z_seen;
}

int32_t encoder_get_um(void) {
    if (!z_seen) return 0;
    int64_t d = (int64_t)((int32_t)raw_count - bias);
    return (int32_t)(d * ENC_PITCH_UM / ENC_COUNTS_PER_REV);
}

int32_t encoder_z_count(void) {
    return z_count;
}

int8_t encoder_direction(void) {
    int32_t c = encoder_get_counts();
    int32_t d = c - last_dir_cnt;
    int8_t dir;
    if (d > DIR_DEADBAND) dir = 1;
    else if (d < -DIR_DEADBAND) dir = -1;
    else dir = 0;
    last_dir_cnt = c;
    return dir;
}

int encoder_raw_state(void) {
    return (gpio_get(ENC_Z_PIN) << 2)
         | (gpio_get(ENC_B_PIN) << 1)
         | gpio_get(ENC_A_PIN);
}
