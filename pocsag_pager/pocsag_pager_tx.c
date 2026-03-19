#include "pocsag_pager_tx.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define POCSAG_SYNC_WORD     0x7CD215D8UL
#define POCSAG_IDLE_WORD     0x7A89C197UL
#define POCSAG_PREAMBLE_BITS 576
#define POCSAG_BCH_GEN_POLY  0xED200000UL

static uint32_t pcsg_tx_bch_and_parity(uint32_t cw) {
    uint32_t local = cw;
    for(int i = 0; i < 21; i++) {
        if(local & 0x80000000UL) {
            local ^= POCSAG_BCH_GEN_POLY;
        }
        local <<= 1;
    }
    cw |= (local >> 21) & 0x7FEUL;

    uint32_t p = cw;
    p ^= p >> 16;
    p ^= p >> 8;
    p ^= p >> 4;
    p ^= p >> 2;
    p ^= p >> 1;
    if(p & 1) cw |= 1;

    return cw;
}

static uint32_t pcsg_tx_addr_codeword(uint32_t ric) {
    uint32_t cw = 0;
    cw |= ((ric >> 3) & 0x3FFFFUL) << 13;
    cw |= (3U << 11); // function 3 = alphanumeric
    return pcsg_tx_bch_and_parity(cw);
}

static uint32_t pcsg_tx_msg_codeword(uint32_t bits20) {
    uint32_t cw = 0x80000000UL;
    cw |= (bits20 & 0xFFFFFUL) << 11;
    return pcsg_tx_bch_and_parity(cw);
}

static uint8_t pcsg_tx_reverse7(uint8_t c) {
    uint8_t r = 0;
    for(int i = 0; i < 7; i++) {
        r = (uint8_t)((r << 1) | (c & 1));
        c >>= 1;
    }
    return r;
}

static void pcsg_tx_write_cw(uint8_t* bits, size_t* pos, uint32_t cw) {
    for(int i = 31; i >= 0; i--) {
        bits[(*pos)++] = (uint8_t)((cw >> i) & 1);
    }
}

PCSGTxData* pcsg_tx_encode(uint32_t ric, const char* message) {
    if(!message) return NULL;

    size_t msg_len = strlen(message);
    size_t total_char_bits = (msg_len + 1) * 7; // +1 for EOT
    uint8_t* char_bits = malloc(total_char_bits);
    if(!char_bits) return NULL;

    size_t cb_pos = 0;
    for(size_t i = 0; i < msg_len; i++) {
        uint8_t rev = pcsg_tx_reverse7((uint8_t)(message[i] & 0x7F));
        for(int b = 6; b >= 0; b--) {
            char_bits[cb_pos++] = (rev >> b) & 1;
        }
    }
    uint8_t eot_rev = pcsg_tx_reverse7(0x04);
    for(int b = 6; b >= 0; b--) {
        char_bits[cb_pos++] = (eot_rev >> b) & 1;
    }

    size_t msg_cw_needed = (cb_pos + 19) / 20;
    uint8_t frame = ric & 7;
    uint8_t addr_slot = frame * 2;

    size_t slots_first = 16 - addr_slot - 1;
    size_t num_batches = 1;
    if(msg_cw_needed > slots_first) {
        size_t remaining = msg_cw_needed - slots_first;
        num_batches += (remaining + 15) / 16;
    }

    size_t total_bits = POCSAG_PREAMBLE_BITS + num_batches * (32 + 16 * 32);

    PCSGTxData* data = malloc(sizeof(PCSGTxData));
    if(!data) {
        free(char_bits);
        return NULL;
    }
    data->bits = malloc(total_bits);
    if(!data->bits) {
        free(char_bits);
        free(data);
        return NULL;
    }
    data->bit_count = 0;
    data->bit_index = 0;

    // Preamble
    for(size_t i = 0; i < POCSAG_PREAMBLE_BITS; i++) {
        data->bits[data->bit_count++] = (i & 1) ? 0 : 1;
    }

    // Batches
    size_t char_bit_offset = 0;
    size_t msg_cw_written = 0;
    bool addr_written = false;

    for(size_t batch = 0; batch < num_batches; batch++) {
        pcsg_tx_write_cw(data->bits, &data->bit_count, POCSAG_SYNC_WORD);

        for(int slot = 0; slot < 16; slot++) {
            if(!addr_written && batch == 0 && slot == addr_slot) {
                pcsg_tx_write_cw(
                    data->bits, &data->bit_count, pcsg_tx_addr_codeword(ric));
                addr_written = true;
            } else if(addr_written && msg_cw_written < msg_cw_needed) {
                uint32_t m20 = 0;
                for(int b = 19; b >= 0; b--) {
                    if(char_bit_offset < cb_pos) {
                        if(char_bits[char_bit_offset]) {
                            m20 |= (1UL << b);
                        }
                        char_bit_offset++;
                    }
                }
                pcsg_tx_write_cw(
                    data->bits, &data->bit_count, pcsg_tx_msg_codeword(m20));
                msg_cw_written++;
            } else {
                pcsg_tx_write_cw(data->bits, &data->bit_count, POCSAG_IDLE_WORD);
            }
        }
    }

    free(char_bits);
    return data;
}

void pcsg_tx_data_free(PCSGTxData* data) {
    if(data) {
        if(data->bits) free(data->bits);
        free(data);
    }
}
