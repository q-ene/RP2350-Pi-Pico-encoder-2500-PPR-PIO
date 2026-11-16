#pragma once

#include <stdint.h>
#include <stdbool.h>

// Распиновка — поменяйте под свою разводку
#define ENC_A_PIN 2
#define ENC_B_PIN 3
#define ENC_Z_PIN 7

// Настройки под станок
#define ENC_COUNTS_PER_REV 10000   // отсчётов декодера на оборот вала (CPR)
#define ENC_PITCH_UM       2000    // ход винта, мкм на оборот (2 мм)

void encoder_init(void);

// Полный счётчик отсчётов с момента старта (может быть отрицательным)
int32_t encoder_get_counts(void);

// Позиция в мкм от последней метки Z (без метки — 0)
int32_t encoder_get_um(void);

// Принятых меток Z с момента старта
int32_t encoder_z_count(void);

// Направление вращения: 1 / -1 / 0 (стоит)
int8_t encoder_direction(void);

// Метка Z уже поймана?
bool encoder_home_seen(void);

// Сырое состояние линий: бит 2 = Z, бит 1 = B, бит 0 = A
int encoder_raw_state(void);
