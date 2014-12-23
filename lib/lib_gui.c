/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_gui.h"
#include "lib_canvas.h"
#include "lib_graphics.h"

const LIB_GUI __LIB_GUI = {
    lib_canvas_create,
    lib_canvas_open,
    lib_canvas_resize,
    lib_canvas_clear,
    lib_canvas_destroy,

    lib_graphics_put_pixel,
    lib_graphics_get_pixel,
    lib_graphics_clear_rect
};
