#include "ui.h"
#include "lvgl.h"
#include "screens/screen_main.h"

void ui_init(void)
{
    screen_main_create();
}

void ui_update(void)
{
    // later: periodic UI updates
}
