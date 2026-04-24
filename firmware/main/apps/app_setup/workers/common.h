/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <smooth_lvgl.hpp>
#include <uitk/short_namespace.hpp>

LV_FONT_DECLARE(BUILTIN_TEXT_FONT);

namespace setup_workers {

inline void apply_button_common_style(uitk::lvgl_cpp::Button& btn)
{
    btn.setBgColor(lv_color_hex(0xB8D3FD));
    btn.setBorderWidth(0);
    btn.setShadowWidth(0);
    btn.setRadius(18);
    btn.label().setTextFont(&BUILTIN_TEXT_FONT);
    btn.label().setTextColor(lv_color_hex(0x26206A));
}

}  // namespace setup_workers
