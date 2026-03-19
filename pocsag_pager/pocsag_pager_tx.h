#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* bits;
    size_t bit_count;
    size_t bit_index;
} PCSGTxData;

PCSGTxData* pcsg_tx_encode(uint32_t ric, const char* message);
void pcsg_tx_data_free(PCSGTxData* data);
