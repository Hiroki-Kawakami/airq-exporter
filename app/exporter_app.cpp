#include "exporter_app.hpp"
#include "lvgl.hpp"

void exporter_app() {
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}
