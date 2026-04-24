/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "workers.h"
#include <stackchan/stackchan.h>
#include <apps/common/toast/toast.h>
#include <mooncake_log.h>
#include <assets/assets.h>
#include <hal/hal.h>
#include <vector>
#include <string>

using namespace smooth_ui_toolkit::lvgl_cpp;
using namespace setup_workers;

static std::string _tag = "Setup-System";
LV_FONT_DECLARE(BUILTIN_TEXT_FONT);

struct TimezoneOption_t {
    std::string name;
    std::string tz_posix;
};

static const std::vector<TimezoneOption_t> _timezone_list = {
    {"贝克岛 (UTC-12)", "BIT12"},  {"中途岛 (UTC-11)", "SST11"}, {"檀香山 (UTC-10)", "HST10"},
    {"阿拉斯加 (UTC-9)", "AKST9"}, {"洛杉矶 (UTC-8)", "PST8"},  {"丹佛 (UTC-7)", "MST7"},
    {"芝加哥 (UTC-6)", "CST6"},   {"纽约 (UTC-5)", "EST5"},    {"哈利法克斯 (UTC-4)", "AST4"},
    {"圣保罗 (UTC-3)", "BRT3"},   {"南乔治亚岛 (UTC-2)", "GST2"}, {"亚速尔群岛 (UTC-1)", "AZOT1"},
    {"伦敦 (UTC+0)", "GMT0"},     {"柏林 (UTC+1)", "CET-1"},   {"开罗 (UTC+2)", "EET-2"},
    {"莫斯科 (UTC+3)", "MSK-3"},  {"迪拜 (UTC+4)", "GST-4"},   {"卡拉奇 (UTC+5)", "PKT-5"},
    {"达卡 (UTC+6)", "BST-6"},    {"曼谷 (UTC+7)", "ICT-7"},   {"北京 (UTC+8)", "CST-8"},
    {"东京 (UTC+9)", "JST-9"},    {"悉尼 (UTC+10)", "AEST-10"}, {"努美阿 (UTC+11)", "SBT-11"},
    {"奥克兰 (UTC+12)", "NZST-12"}, {"斐济 (UTC+13)", "FJT-13"}, {"莱恩群岛 (UTC+14)", "LINT-14"}};

VolumeSetupWorker::VolumeSetupWorker()
{
    mclog::info("VolumeSetupWorker start");

    for (int volume = 0; volume <= 100; volume += 5) {
        _volume_levels.push_back(volume);
    }

    uint8_t current_volume = GetHAL().getSpeakerVolume();
    int current_index      = _volume_levels.size() - 1;
    for (size_t i = 0; i < _volume_levels.size(); i++) {
        if (_volume_levels[i] >= current_volume) {
            current_index = static_cast<int>(i);
            break;
        }
    }

    _panel = std::make_unique<Container>(lv_screen_active());
    _panel->setBgColor(lv_color_hex(0xEDF4FF));
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setBorderWidth(0);
    _panel->setSize(320, 240);
    _panel->setRadius(0);
    _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _label_volume = std::make_unique<Label>(*_panel);
    _label_volume->setText(fmt::format("{}%", _volume_levels[current_index]));
    _label_volume->setTextFont(&lv_font_montserrat_24);
    _label_volume->setTextColor(lv_color_hex(0x26206A));
    _label_volume->align(LV_ALIGN_CENTER, 0, -70);

    _slider = std::make_unique<Slider>(*_panel);
    _slider->align(LV_ALIGN_CENTER, 0, -12);
    _slider->setRange(0, _volume_levels.size() - 1);
    _slider->setSize(250, 18);
    _slider->setBgColor(lv_color_hex(0x615B9E), LV_PART_KNOB);
    _slider->setBgColor(lv_color_hex(0x615B9E), LV_PART_INDICATOR);
    _slider->setBgColor(lv_color_hex(0xB8D3FD), LV_PART_MAIN);
    _slider->setBgOpa(255);
    _slider->setValue(current_index);
    _slider->onValueChanged().connect([this](int32_t value) {
        _label_volume->setText(fmt::format("{}%", _volume_levels[value]));
        _target_volume = _volume_levels[value];
    });

    _btn_confirm = std::make_unique<Button>(*_panel);
    apply_button_common_style(*_btn_confirm);
    _btn_confirm->align(LV_ALIGN_CENTER, 0, 60);
    _btn_confirm->setSize(150, 50);
    _btn_confirm->label().setText("确定");
    _btn_confirm->onClick().connect([this]() { _is_done = true; });
}

VolumeSetupWorker::~VolumeSetupWorker()
{
    auto volume = _volume_levels[_slider->getValue()];
    mclog::tagInfo(_tag, "final volume: {}", volume);
    GetHAL().setSpeakerVolume(volume, true);
}

void VolumeSetupWorker::update()
{
    if (_target_volume != -1) {
        GetHAL().setSpeakerVolume(_target_volume, false);
        _target_volume = -1;
    }
}

TimezoneWorker::TimezoneWorker()
{
    _panel = std::make_unique<uitk::lvgl_cpp::Container>(lv_screen_active());
    _panel->setPadding(0, 0, 0, 0);
    _panel->setBgColor(lv_color_hex(0xEDF4FF));
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setBorderWidth(0);
    _panel->setSize(320, 240);
    _panel->setRadius(0);

    _label = std::make_unique<uitk::lvgl_cpp::Label>(_panel->get());
    _label->setText("时区");
    _label->setTextFont(&BUILTIN_TEXT_FONT);
    _label->setTextColor(lv_color_hex(0x26206A));
    _label->align(LV_ALIGN_CENTER, 0, -100);

    // Timezone list
    std::string options;
    for (const auto& tz : _timezone_list) {
        options += tz.name + "\n";
    }
    // Remove last newline
    if (!options.empty()) {
        options.pop_back();
    }

    _roller = std::make_unique<uitk::lvgl_cpp::Roller>(_panel->get());
    _roller->setSize(210, 188);
    _roller->setOptions(options.c_str());
    _roller->align(LV_ALIGN_CENTER, -40, 16);
    _roller->setTextFont(&BUILTIN_TEXT_FONT);
    _roller->setTextColor(lv_color_hex(0x26206A));
    _roller->setBgColor(lv_color_hex(0xB8D3FD));
    _roller->setRadius(18);
    _roller->setShadowWidth(0);
    _roller->setBorderWidth(0);
    _roller->setBgColor(lv_color_hex(0x615B9E), LV_PART_SELECTED);

    // Set current selection
    std::string current_tz = GetHAL().getTimezone();
    int utc0_index         = -1;
    for (size_t i = 0; i < _timezone_list.size(); ++i) {
        if (current_tz == _timezone_list[i].tz_posix) {
            _roller->setSelected(i, LV_ANIM_OFF);
            utc0_index = -1;
            break;
        } else if (_timezone_list[i].tz_posix == "GMT0") {
            utc0_index = i;
        }
    }

    if (utc0_index >= 0) {
        // Default to UTC+0
        _roller->setSelected(utc0_index, LV_ANIM_OFF);
    }

    _btn_confirm = std::make_unique<uitk::lvgl_cpp::Button>(_panel->get());
    _btn_confirm->label().setText("确定");
    _btn_confirm->label().setTextFont(&BUILTIN_TEXT_FONT);
    _btn_confirm->setSize(60, 110);
    _btn_confirm->align(LV_ALIGN_CENTER, 115, 40);
    _btn_confirm->onClick().connect([&]() { _confirm_flag = true; });
    _btn_confirm->setRadius(18);
    _btn_confirm->setShadowWidth(0);
    _btn_confirm->setBgColor(lv_color_hex(0x615B9E));
}

TimezoneWorker::~TimezoneWorker()
{
}

void TimezoneWorker::update()
{
    if (_confirm_flag) {
        _confirm_flag = false;

        uint16_t selected_id = _roller->getSelected();
        if (selected_id < _timezone_list.size()) {
            const auto& selected_option = _timezone_list[selected_id];
            GetHAL().setTimezone(selected_option.tz_posix);

            view::pop_a_toast("时区已设置", view::ToastType::Success);
            mclog::tagInfo(_tag, "timezone set to: {}", selected_option.name);
        }

        _is_done = true;
    }
}

FactoryResetWorker::FactoryResetWorker()
{
    _panel = std::make_unique<uitk::lvgl_cpp::Container>(lv_screen_active());
    _panel->setPadding(0, 0, 0, 0);
    _panel->setBgColor(lv_color_hex(0xEDF4FF));
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setBorderWidth(0);
    _panel->setSize(320, 240);
    _panel->setRadius(0);

    // Title
    _label_title = std::make_unique<uitk::lvgl_cpp::Label>(_panel->get());
    _label_title->setText("恢复出厂");
    _label_title->setTextFont(&BUILTIN_TEXT_FONT);
    _label_title->setTextColor(lv_color_hex(0x26206A));
    _label_title->align(LV_ALIGN_CENTER, 0, -80);

    // Info
    _label_info = std::make_unique<uitk::lvgl_cpp::Label>(_panel->get());
    _label_info->setTextFont(&BUILTIN_TEXT_FONT);
    _label_info->setTextColor(lv_color_hex(0x26206A));
    _label_info->align(LV_ALIGN_CENTER, 0, -20);
    _label_info->setTextAlign(LV_TEXT_ALIGN_CENTER);
    _label_info->setWidth(280);

    // Cancel Button
    _btn_cancel = std::make_unique<uitk::lvgl_cpp::Button>(_panel->get());
    apply_button_common_style(*_btn_cancel);
    _btn_cancel->align(LV_ALIGN_CENTER, -72, 60);
    _btn_cancel->setSize(112, 48);
    _btn_cancel->label().setText("取消");
    _btn_cancel->label().setTextFont(&BUILTIN_TEXT_FONT);
    _btn_cancel->onClick().connect([this]() { _cancel_flag = true; });

    // Confirm Button
    _btn_confirm = std::make_unique<uitk::lvgl_cpp::Button>(_panel->get());
    apply_button_common_style(*_btn_confirm);
    _btn_confirm->align(LV_ALIGN_CENTER, 72, 60);
    _btn_confirm->setSize(112, 48);
    _btn_confirm->label().setText("确定");
    _btn_confirm->label().setTextFont(&BUILTIN_TEXT_FONT);
    _btn_confirm->onClick().connect([this]() { _confirm_flag = true; });

    update_ui();
}

FactoryResetWorker::~FactoryResetWorker()
{
}

void FactoryResetWorker::update()
{
    if (_cancel_flag) {
        _is_done = true;
        return;
    }

    if (_confirm_flag) {
        _confirm_flag = false;
        _confirm_count++;

        if (_confirm_count >= 3) {
            mclog::tagInfo(_tag, "factory reset triggered");

            _btn_cancel.reset();
            _btn_confirm.reset();
            _label_title.reset();

            _label_info->setText("正在恢复出厂设置...\n请勿断电。");
            _label_info->align(LV_ALIGN_CENTER, 0, 0);

            GetHAL().lvglUnlock();
            GetHAL().delay(200);
            GetHAL().factoryReset();

            while (1) {
                GetHAL().delay(200);
            }

        } else {
            update_ui();
        }
    }
}

void FactoryResetWorker::update_ui()
{
    if (_confirm_count == 0) {
        _label_info->setText("将全部设置恢复为出厂默认？\n此操作无法撤销。");
        _btn_confirm->label().setText("重置");
        _btn_confirm->setBgColor(lv_color_hex(0xFFB8B8));
    } else if (_confirm_count == 1) {
        _label_info->setText("确认继续吗？\n所有用户数据都会丢失！");
        _btn_confirm->label().setText("继续");
        _btn_confirm->setBgColor(lv_color_hex(0xFF8888));
    } else if (_confirm_count == 2) {
        _label_info->setText("最后警告！\n按“确定”将清除全部数据。");
        _btn_confirm->label().setText("确定");
        _btn_confirm->setBgColor(lv_color_hex(0xFF4444));
    }
}
