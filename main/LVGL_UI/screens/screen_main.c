#include "screen_main.h"
#include "lvgl.h"

static lv_obj_t *value_label;

void screen_main_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Root container */
    lv_obj_t *root = lv_obj_create(scr);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_center(root);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        root,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_set_style_pad_all(root, 4, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    /* ---------- Top bar ---------- */
    lv_obj_t *top = lv_obj_create(root);
    lv_obj_set_size(top, lv_pct(100), 28);
    lv_obj_set_style_radius(top, 8, 0);
    lv_obj_set_style_bg_color(top, lv_palette_main(LV_PALETTE_BLUE), 0);

    lv_obj_t *title = lv_label_create(top);
    lv_label_set_text(title, "ESP32-C6");
    lv_obj_center(title);

    /* ---------- Main card ---------- */
    lv_obj_t *card = lv_obj_create(root);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, 0);          /* let flex sizing handle height */
    lv_obj_set_flex_grow(card, 1);       /* take all remaining vertical space */
    lv_obj_set_style_pad_all(card, 6, 0);

    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_bg_color(card, lv_palette_main(LV_PALETTE_GREY), 0);

    value_label = lv_label_create(card);
    lv_label_set_text(value_label, "READY");
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_16, 0);
    lv_obj_center(value_label);

    /* ---------- Footer ---------- */
    lv_obj_t *footer = lv_label_create(root);
    lv_label_set_text(footer, "System OK");
}

void screen_main_update(void)
{
    /* Example dynamic update later */
    // lv_label_set_text(value_label, "123");
}

void screen_main_destroy(void)
{
    /* Optional cleanup */
}
