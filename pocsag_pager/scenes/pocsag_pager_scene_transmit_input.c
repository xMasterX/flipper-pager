#include "../pocsag_pager_app_i.h"

static void pocsag_pager_scene_transmit_input_done(void* context) {
    POCSAGPagerApp* app = context;

    if(app->tx_edit_field == 0) {
        // RIC
        strncpy(app->tx_ric_str, app->tx_text_buf, sizeof(app->tx_ric_str) - 1);
        app->tx_ric_str[sizeof(app->tx_ric_str) - 1] = '\0';
    } else {
        // Message
        strncpy(app->tx_msg_str, app->tx_text_buf, sizeof(app->tx_msg_str) - 1);
        app->tx_msg_str[sizeof(app->tx_msg_str) - 1] = '\0';
    }

    scene_manager_previous_scene(app->scene_manager);
}

void pocsag_pager_scene_transmit_input_on_enter(void* context) {
    POCSAGPagerApp* app = context;

    text_input_reset(app->text_input);

    if(app->tx_edit_field == 0) {
        text_input_set_header_text(app->text_input, "Enter RIC (address):");
        strncpy(app->tx_text_buf, app->tx_ric_str, sizeof(app->tx_text_buf) - 1);
        text_input_set_result_callback(
            app->text_input,
            pocsag_pager_scene_transmit_input_done,
            app,
            app->tx_text_buf,
            sizeof(app->tx_ric_str),
            false);
    } else {
        text_input_set_header_text(app->text_input, "Enter message:");
        strncpy(app->tx_text_buf, app->tx_msg_str, sizeof(app->tx_text_buf) - 1);
        text_input_set_result_callback(
            app->text_input,
            pocsag_pager_scene_transmit_input_done,
            app,
            app->tx_text_buf,
            sizeof(app->tx_msg_str),
            false);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, POCSAGPagerViewTextInput);
}

bool pocsag_pager_scene_transmit_input_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void pocsag_pager_scene_transmit_input_on_exit(void* context) {
    POCSAGPagerApp* app = context;
    text_input_reset(app->text_input);
}
