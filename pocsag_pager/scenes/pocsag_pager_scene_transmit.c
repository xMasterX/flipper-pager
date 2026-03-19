#include "../pocsag_pager_app_i.h"
#include "../pocsag_pager_tx.h"
#include <pocsag_pager_trx_icons.h>

#include <stdlib.h>

static const uint32_t tx_freq_values[] = {
    439987500, 439980000, 433920000, 433875000, 446000000,
};
static const char* tx_freq_labels[] = {
    "439.9875", "439.9800", "433.9200", "433.8750", "446.0000",
};
#define TX_FREQ_COUNT 5

typedef enum {
    TransmitItemRic,
    TransmitItemMsg,
    TransmitItemFreq,
    TransmitItemSend,
} TransmitItem;

// TX async callback context
typedef struct {
    PCSGTxData* data;
    uint32_t baud_duration_us;
} PCSGTxContext;

static PCSGTxContext tx_ctx;

static LevelDuration pcsg_tx_callback(void* context) {
    PCSGTxContext* ctx = context;

    if(ctx->data->bit_index >= ctx->data->bit_count) {
        return level_duration_reset();
    }

    uint8_t bit = ctx->data->bits[ctx->data->bit_index++];

    // Inverted polarity for CC1101 compatibility with standard pagers
    bool level = (bit != 0);

    return level_duration_make(level, ctx->baud_duration_us);
}

static void pocsag_pager_scene_transmit_freq_changed(VariableItem* item) {
    POCSAGPagerApp* app = variable_item_get_context(item);
    app->tx_freq_index = variable_item_get_current_value_index(item);
    app->tx_frequency = tx_freq_values[app->tx_freq_index];
    variable_item_set_current_value_text(item, tx_freq_labels[app->tx_freq_index]);
}

static void pocsag_pager_scene_transmit_item_enter(void* context, uint32_t index) {
    POCSAGPagerApp* app = context;
    if(index == TransmitItemRic) {
        app->tx_edit_field = 0;
        view_dispatcher_send_custom_event(
            app->view_dispatcher, PCSGCustomEventTransmitEditRic);
    } else if(index == TransmitItemMsg) {
        app->tx_edit_field = 1;
        view_dispatcher_send_custom_event(
            app->view_dispatcher, PCSGCustomEventTransmitEditMsg);
    } else if(index == TransmitItemSend) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, PCSGCustomEventTransmitSend);
    }
}

void pocsag_pager_scene_transmit_on_enter(void* context) {
    POCSAGPagerApp* app = context;
    VariableItemList* list = app->variable_item_list;
    variable_item_list_reset(list);

    VariableItem* item_ric = variable_item_list_add(list, "RIC", 0, NULL, app);
    variable_item_set_current_value_text(item_ric, app->tx_ric_str);

    VariableItem* item_msg = variable_item_list_add(list, "Message", 0, NULL, app);
    variable_item_set_current_value_text(item_msg, app->tx_msg_str);

    VariableItem* item_freq = variable_item_list_add(
        list, "Freq MHz", TX_FREQ_COUNT, pocsag_pager_scene_transmit_freq_changed, app);
    variable_item_set_current_value_index(item_freq, app->tx_freq_index);
    variable_item_set_current_value_text(item_freq, tx_freq_labels[app->tx_freq_index]);

    variable_item_list_add(list, ">> SEND <<", 0, NULL, app);

    variable_item_list_set_enter_callback(
        list, pocsag_pager_scene_transmit_item_enter, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, POCSAGPagerViewVariableItemList);
}

bool pocsag_pager_scene_transmit_on_event(void* context, SceneManagerEvent event) {
    POCSAGPagerApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == PCSGCustomEventTransmitEditRic ||
           event.event == PCSGCustomEventTransmitEditMsg) {
            scene_manager_next_scene(app->scene_manager, POCSAGPagerSceneTransmitInput);
            consumed = true;

        } else if(event.event == PCSGCustomEventTransmitSend) {
            // Stop any active RX first
            if(app->txrx->txrx_state == PCSGTxRxStateRx) {
                pcsg_rx_end(app);
            }
            if(app->txrx->txrx_state == PCSGTxRxStateSleep) {
                subghz_devices_idle(app->txrx->radio_device);
                app->txrx->txrx_state = PCSGTxRxStateIDLE;
            }

            // Show transmitting widget with scanning image
            widget_reset(app->widget);
            widget_add_icon_element(app->widget, 0, 0, &I_Scanning_123x52);
            widget_add_string_element(
                app->widget, 63, 46, AlignLeft, AlignBottom,
                FontPrimary, "Sending...");

            char info[48];
            snprintf(info, sizeof(info), "RIC:%s  %s MHz",
                     app->tx_ric_str, tx_freq_labels[app->tx_freq_index]);
            widget_add_string_element(
                app->widget, 44, 62, AlignLeft, AlignBottom,
                FontSecondary, info);

            view_dispatcher_switch_to_view(app->view_dispatcher, POCSAGPagerViewWidget);

            // Encode POCSAG
            uint32_t ric = strtoul(app->tx_ric_str, NULL, 10);
            PCSGTxData* tx_data = pcsg_tx_encode(ric, app->tx_msg_str);

            if(tx_data) {
                tx_ctx.data = tx_data;
                tx_ctx.baud_duration_us = 833; // 1200 baud

                // Use built-in 2FSK preset — custom presets don't work for async TX
                subghz_devices_idle(app->txrx->radio_device);
                subghz_devices_load_preset(
                    app->txrx->radio_device,
                    FuriHalSubGhzPreset2FSKDev238Async,
                    NULL);
                subghz_devices_set_frequency(
                    app->txrx->radio_device, app->tx_frequency);

                if(subghz_devices_start_async_tx(
                       app->txrx->radio_device, pcsg_tx_callback, &tx_ctx)) {
                    app->txrx->txrx_state = PCSGTxRxStateTx;
                    notification_message(
                        app->notifications, &sequence_blink_start_magenta);
                } else {
                    pcsg_tx_data_free(tx_data);
                    tx_ctx.data = NULL;
                }
            }
            consumed = true;

        } else if(event.event == PCSGCustomEventTransmitDone) {
            // TX complete — stop and return radio to idle
            if(app->txrx->txrx_state == PCSGTxRxStateTx) {
                subghz_devices_stop_async_tx(app->txrx->radio_device);
                subghz_devices_idle(app->txrx->radio_device);
                app->txrx->txrx_state = PCSGTxRxStateIDLE;
            }

            if(tx_ctx.data) {
                pcsg_tx_data_free(tx_ctx.data);
                tx_ctx.data = NULL;
            }

            notification_message(app->notifications, &sequence_blink_stop);
            notification_message(app->notifications, &sequence_success);

            widget_reset(app->widget);
            widget_add_string_element(
                app->widget, 64, 25, AlignCenter, AlignCenter,
                FontPrimary, "Sent!");
            widget_add_string_element(
                app->widget, 64, 45, AlignCenter, AlignCenter,
                FontSecondary, "Press Back");
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        // Poll TX completion via tick (100ms interval from view_dispatcher)
        if(app->txrx->txrx_state == PCSGTxRxStateTx) {
            if(subghz_devices_is_async_complete_tx(app->txrx->radio_device)) {
                view_dispatcher_send_custom_event(
                    app->view_dispatcher, PCSGCustomEventTransmitDone);
            }
        }
    }

    return consumed;
}

void pocsag_pager_scene_transmit_on_exit(void* context) {
    POCSAGPagerApp* app = context;

    // Emergency stop if TX still active
    if(app->txrx->txrx_state == PCSGTxRxStateTx) {
        subghz_devices_stop_async_tx(app->txrx->radio_device);
        subghz_devices_idle(app->txrx->radio_device);
        app->txrx->txrx_state = PCSGTxRxStateIDLE;
    }

    if(tx_ctx.data) {
        pcsg_tx_data_free(tx_ctx.data);
        tx_ctx.data = NULL;
    }

    notification_message(app->notifications, &sequence_blink_stop);
    variable_item_list_reset(app->variable_item_list);
    widget_reset(app->widget);
}
