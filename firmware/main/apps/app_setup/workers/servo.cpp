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
#include <array>

using namespace smooth_ui_toolkit::lvgl_cpp;
using namespace setup_workers;

static std::string _tag = "Setup-Servo";
LV_FONT_DECLARE(BUILTIN_TEXT_FONT);

/**
 * @brief
 *
 */
class PageTips : public WorkerBase {
public:
    PageTips()
    {
        _panel = std::make_unique<Container>(lv_screen_active());
        _panel->setBgColor(lv_color_hex(0xEDF4FF));
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setBorderWidth(0);
        _panel->setSize(320, 240);
        _panel->setRadius(0);

        _title = std::make_unique<Label>(lv_screen_active());
        _title->setTextFont(&BUILTIN_TEXT_FONT);
        _title->setTextColor(lv_color_hex(0x7E7B9C));
        _title->align(LV_ALIGN_TOP_MID, 0, 13);
        _title->setText("归中校准");

        _img                            = std::make_unique<Image>(lv_screen_active());
        _img_setup_stackchan_front_view = assets::get_image("setup_stackchan_front_view.bin");
        _img->setSrc(&_img_setup_stackchan_front_view);
        _img->align(LV_ALIGN_CENTER, -74, 15);

        _btn_next = std::make_unique<Button>(lv_screen_active());
        apply_button_common_style(*_btn_next);
        _btn_next->align(LV_ALIGN_CENTER, 79, 73);
        _btn_next->setSize(120, 48);
        _btn_next->label().setText("继续");
        _btn_next->label().setTextFont(&BUILTIN_TEXT_FONT);
        _btn_next->onClick().connect([this]() { _is_done = true; });

        _info = std::make_unique<Label>(lv_screen_active());
        _info->setTextFont(&BUILTIN_TEXT_FONT);
        _info->setTextColor(lv_color_hex(0x26206A));
        _info->align(LV_ALIGN_TOP_LEFT, 185, 56);
        _info->setTextAlign(LV_TEXT_ALIGN_LEFT);
        _info->setText("让 StackChan\n正视前方。");
    }

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _panel;
    std::unique_ptr<uitk::lvgl_cpp::Label> _title;
    std::unique_ptr<uitk::lvgl_cpp::Image> _img;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_next;
    std::unique_ptr<uitk::lvgl_cpp::Label> _info;
    lv_image_dsc_t _img_setup_stackchan_front_view;
};

/**
 * @brief
 *
 */
class PageCalibration : public WorkerBase {
public:
    PageCalibration()
    {
        _panel = std::make_unique<Container>(lv_screen_active());
        _panel->setBgColor(lv_color_hex(0xFFFFFF));
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setBorderWidth(0);
        _panel->setSize(320, 240);
        _panel->setRadius(0);
        _panel->setFlexFlow(LV_FLEX_FLOW_COLUMN);
        _panel->setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        _panel->setPadding(30, 30, 0, 0);
        _panel->setPadRow(24);

        _btn_go_home = std::make_unique<Button>(*_panel);
        apply_button_common_style(*_btn_go_home);
        _btn_go_home->setSize(290, 70);
        _btn_go_home->label().setText("回到归中");
        _btn_go_home->onClick().connect([this]() { _go_home_flag = true; });

        _btn_confirm = std::make_unique<Button>(*_panel);
        apply_button_common_style(*_btn_confirm);
        _btn_confirm->setSize(290, 80);
        _btn_confirm->setBgColor(lv_color_hex(0xFFDF9A));
        _btn_confirm->label().setText("将当前位置设为\n归中位置");
        _btn_confirm->label().setTextAlign(LV_TEXT_ALIGN_CENTER);
        _btn_confirm->label().setTextColor(lv_color_hex(0x47330A));
        _btn_confirm->onClick().connect([this]() { _confirm_flag = true; });

        _btn_reset_default = std::make_unique<Button>(*_panel);
        apply_button_common_style(*_btn_reset_default);
        _btn_reset_default->setSize(290, 80);
        _btn_reset_default->setBgColor(lv_color_hex(0xBAE4BA));
        _btn_reset_default->label().setText("恢复默认\n归中位置");
        _btn_reset_default->label().setTextAlign(LV_TEXT_ALIGN_CENTER);
        _btn_reset_default->label().setTextColor(lv_color_hex(0x233B23));
        _btn_reset_default->onClick().connect([this]() { _reset_default_flag = true; });

        _btn_quit = std::make_unique<Button>(*_panel);
        apply_button_common_style(*_btn_quit);
        _btn_quit->setSize(230, 55);
        _btn_quit->label().setText("完成");
        _btn_quit->onClick().connect([this]() { _is_done = true; });

        auto& motion = GetStackChan().motion();
        motion.setAutoAngleSyncEnabled(true);
    }

    void update() override
    {
        if (_confirm_flag) {
            _confirm_flag = false;

            mclog::tagInfo(_tag, "set current angle as zero");

            auto& motion = GetStackChan().motion();
            motion.yawServo().setCurrentAngleAsZero();
            motion.pitchServo().setCurrentAngleAsZero();

            view::pop_a_toast("已设置归中位置", view::ToastType::Success);
        }

        if (_go_home_flag) {
            _go_home_flag = false;

            view::pop_a_toast("正在回到归中", view::ToastType::Warning);
            mclog::tagInfo(_tag, "go home");

            auto& motion = GetStackChan().motion();
            motion.goHome(666);
        }

        if (_reset_default_flag) {
            _reset_default_flag = false;

            mclog::tagInfo(_tag, "home reset");

            auto& motion = GetStackChan().motion();
            motion.yawServo().resetZeroCalibration();
            motion.pitchServo().resetZeroCalibration();

            view::pop_a_toast("已恢复默认归中位置", view::ToastType::Success);
        }
    }

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _panel;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_quit;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_confirm;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_go_home;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_reset_default;
    bool _confirm_flag       = false;
    bool _go_home_flag       = false;
    bool _reset_default_flag = false;
};

ZeroCalibrationWorker::ZeroCalibrationWorker()
{
    _page_tips = std::make_unique<PageTips>();
}

void ZeroCalibrationWorker::update()
{
    // Page tips
    if (_page_tips) {
        _page_tips->update();
        if (_page_tips->isDone()) {
            _page_tips.reset();
            _page_calibration = std::make_unique<PageCalibration>();
        }
    }
    // Page calibration
    else if (_page_calibration) {
        _page_calibration->update();
        if (_page_calibration->isDone()) {
            _page_calibration.reset();
            _is_done = true;
        }
    }
}

class PageServoTips : public WorkerBase {
public:
    PageServoTips()
    {
        _panel = std::make_unique<Container>(lv_screen_active());
        _panel->setBgColor(lv_color_hex(0xEDF4FF));
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setBorderWidth(0);
        _panel->setSize(320, 240);
        _panel->setRadius(0);

        _title = std::make_unique<Label>(lv_screen_active());
        _title->setTextFont(&BUILTIN_TEXT_FONT);
        _title->setTextColor(lv_color_hex(0x7E7B9C));
        _title->align(LV_ALIGN_TOP_MID, 0, 13);
        _title->setText("舵机测试");

        _info = std::make_unique<Label>(lv_screen_active());
        _info->setWidth(280);
        _info->setTextFont(&BUILTIN_TEXT_FONT);
        _info->setTextColor(lv_color_hex(0x26206A));
        _info->align(LV_ALIGN_CENTER, 0, -18);
        _info->setTextAlign(LV_TEXT_ALIGN_CENTER);
        _info->setText("请把我放在平稳桌面上，\n并移开双手。\n\n如有需要，\n请从底部 USB-C 供电。");

        _btn_skip = std::make_unique<Button>(lv_screen_active());
        apply_button_common_style(*_btn_skip);
        _btn_skip->align(LV_ALIGN_CENTER, -72, 72);
        _btn_skip->setSize(112, 48);
        _btn_skip->setBgColor(lv_color_hex(0xD4D9E0));
        _btn_skip->label().setText("跳过");
        _btn_skip->label().setTextFont(&BUILTIN_TEXT_FONT);
        _btn_skip->label().setTextColor(lv_color_hex(0x525064));
        _btn_skip->onClick().connect([this]() { _is_skip_clicked = true; });

        _btn_start = std::make_unique<Button>(lv_screen_active());
        apply_button_common_style(*_btn_start);
        _btn_start->align(LV_ALIGN_CENTER, 72, 72);
        _btn_start->setSize(112, 48);
        _btn_start->label().setText("开始");
        _btn_start->label().setTextFont(&BUILTIN_TEXT_FONT);
        _btn_start->onClick().connect([this]() { _is_start_clicked = true; });
    }

    bool isSkipClicked() const
    {
        return _is_skip_clicked;
    }

    bool isStartClicked() const
    {
        return _is_start_clicked;
    }

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _panel;
    std::unique_ptr<uitk::lvgl_cpp::Label> _title;
    std::unique_ptr<uitk::lvgl_cpp::Label> _info;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_skip;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_start;
    bool _is_skip_clicked  = false;
    bool _is_start_clicked = false;
};

class PageServoTest : public WorkerBase {
public:
    PageServoTest()
    {
        _panel = std::make_unique<Container>(lv_screen_active());
        _panel->setBgColor(lv_color_hex(0xEDF4FF));
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setBorderWidth(0);
        _panel->setSize(320, 240);
        _panel->setRadius(0);

        _title = std::make_unique<Label>(lv_screen_active());
        _title->setTextFont(&BUILTIN_TEXT_FONT);
        _title->setTextColor(lv_color_hex(0x7E7B9C));
        _title->align(LV_ALIGN_TOP_MID, 0, 13);
        _title->setText("舵机测试");

        _info = std::make_unique<Label>(lv_screen_active());
        _info->setWidth(280);
        _info->setTextFont(&BUILTIN_TEXT_FONT);
        _info->setTextColor(lv_color_hex(0x26206A));
        _info->align(LV_ALIGN_CENTER, 0, -12);
        _info->setTextAlign(LV_TEXT_ALIGN_CENTER);
        _info->setText("准备中...");

        auto& motion = GetStackChan().motion();
        motion.setAutoAngleSyncEnabled(true);
    }

    void update() override
    {
        if (_current_step >= _steps.size()) {
            _is_done = true;
            return;
        }

        const auto now = GetHAL().millis();
        if (!_step_started) {
            run_step(_steps[_current_step]);
            _step_started    = true;
            _step_start_tick = now;
            return;
        }

        if (now - _step_start_tick >= _step_delay_ms) {
            _current_step++;
            _step_started = false;
        }
    }

private:
    enum class Step {
        GoHome1,
        Left90,
        GoHome2,
        Right90,
        GoHome3,
        Up90,
        GoHome4,
    };

    void run_step(Step step)
    {
        auto& motion = GetStackChan().motion();
        switch (step) {
            case Step::GoHome1:
            case Step::GoHome2:
            case Step::GoHome3:
            case Step::GoHome4:
                _info->setText("正在回到\n归中位置...");
                motion.goHome(_move_speed);
                break;
            case Step::Left90:
                _info->setText("正在向左移动...");
                motion.moveWithSpeed(900, 0, _move_speed);
                break;
            case Step::Right90:
                _info->setText("正在向右移动...");
                motion.moveWithSpeed(-900, 0, _move_speed);
                break;
            case Step::Up90:
                _info->setText("正在向上移动...");
                motion.moveWithSpeed(0, 900, _move_speed);
                break;
        }
    }

    std::unique_ptr<uitk::lvgl_cpp::Container> _panel;
    std::unique_ptr<uitk::lvgl_cpp::Label> _title;
    std::unique_ptr<uitk::lvgl_cpp::Label> _info;
    static constexpr uint32_t _step_delay_ms = 1800;
    static constexpr int _move_speed         = 500;
    const std::array<Step, 7> _steps         = {
        Step::GoHome1, Step::Left90, Step::GoHome2, Step::Right90, Step::GoHome3, Step::Up90, Step::GoHome4,
    };
    size_t _current_step      = 0;
    uint32_t _step_start_tick = 0;
    bool _step_started        = false;
};

class PageServoDone : public WorkerBase {
public:
    PageServoDone()
    {
        _panel = std::make_unique<Container>(lv_screen_active());
        _panel->setBgColor(lv_color_hex(0xEDF4FF));
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setBorderWidth(0);
        _panel->setSize(320, 240);
        _panel->setRadius(0);

        _title = std::make_unique<Label>(lv_screen_active());
        _title->setTextFont(&BUILTIN_TEXT_FONT);
        _title->setTextColor(lv_color_hex(0x7E7B9C));
        _title->align(LV_ALIGN_TOP_MID, 0, 13);
        _title->setText("舵机测试");

        _info = std::make_unique<Label>(lv_screen_active());
        _info->setWidth(280);
        _info->setTextFont(&BUILTIN_TEXT_FONT);
        _info->setTextColor(lv_color_hex(0x26206A));
        _info->align(LV_ALIGN_CENTER, 0, -16);
        _info->setTextAlign(LV_TEXT_ALIGN_CENTER);
        _info->setText("舵机测试完成。");

        _btn_retest = std::make_unique<Button>(lv_screen_active());
        apply_button_common_style(*_btn_retest);
        _btn_retest->align(LV_ALIGN_CENTER, -72, 67);
        _btn_retest->setSize(112, 48);
        _btn_retest->setBgColor(lv_color_hex(0xD4D9E0));
        _btn_retest->label().setText("再测一次");
        _btn_retest->label().setTextFont(&BUILTIN_TEXT_FONT);
        _btn_retest->label().setTextColor(lv_color_hex(0x525064));
        _btn_retest->onClick().connect([this]() { _is_retest_clicked = true; });

        _btn_next = std::make_unique<Button>(lv_screen_active());
        apply_button_common_style(*_btn_next);
        _btn_next->align(LV_ALIGN_CENTER, 72, 67);
        _btn_next->setSize(112, 48);
        _btn_next->label().setText("下一步");
        _btn_next->label().setTextFont(&BUILTIN_TEXT_FONT);
        _btn_next->onClick().connect([this]() { _is_next_clicked = true; });
    }

    bool isRetestClicked() const
    {
        return _is_retest_clicked;
    }

    bool isNextClicked() const
    {
        return _is_next_clicked;
    }

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _panel;
    std::unique_ptr<uitk::lvgl_cpp::Label> _title;
    std::unique_ptr<uitk::lvgl_cpp::Label> _info;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_retest;
    std::unique_ptr<uitk::lvgl_cpp::Button> _btn_next;
    bool _is_retest_clicked = false;
    bool _is_next_clicked   = false;
};

ServoTestWorker::ServoTestWorker()
{
    _page_tips = std::make_unique<PageServoTips>();
}

void ServoTestWorker::update()
{
    if (_page_tips) {
        auto* page = static_cast<PageServoTips*>(_page_tips.get());
        if (page->isSkipClicked()) {
            _page_tips.reset();
            _is_done = true;
        } else if (page->isStartClicked()) {
            _page_tips.reset();
            _page_test = std::make_unique<PageServoTest>();
        }
    } else if (_page_test) {
        _page_test->update();
        if (_page_test->isDone()) {
            _page_test.reset();
            _page_done = std::make_unique<PageServoDone>();
        }
    } else if (_page_done) {
        auto* page = static_cast<PageServoDone*>(_page_done.get());
        if (page->isRetestClicked()) {
            _page_done.reset();
            _page_test = std::make_unique<PageServoTest>();
        } else if (page->isNextClicked()) {
            _page_done.reset();
            _is_done = true;
        }
    }
}

struct RgbColorEntry {
    std::string name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

static const std::vector<RgbColorEntry> _rgb_colors = {{"红色", 255, 0, 0},
                                                       {"绿色", 0, 255, 0},
                                                       {"蓝色", 0, 0, 255},
                                                       {"黄色", 255, 255, 0},
                                                       {"青色", 0, 255, 255},
                                                       {"洋红", 255, 0, 255},
                                                       {"白色", 255, 255, 255},
                                                       {"关闭", 0, 0, 0}};

RgbTestWorker::RgbTestWorker()
{
    _panel = std::make_unique<Container>(lv_screen_active());
    _panel->setBgColor(lv_color_hex(0xFFFFFF));
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setBorderWidth(0);
    _panel->setSize(320, 240);
    _panel->setRadius(0);
    _panel->setFlexFlow(LV_FLEX_FLOW_COLUMN);
    _panel->setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    _panel->setPadding(20, 20, 20, 20);
    _panel->setPadRow(15);

    for (const auto& color : _rgb_colors) {
        auto btn = std::make_unique<Button>(*_panel);
        apply_button_common_style(*btn);
        btn->setSize(200, 50);
        btn->label().setText(color.name);

        uint8_t r = color.r;
        uint8_t g = color.g;
        uint8_t b = color.b;
        btn->onClick().connect([r, g, b]() {
            GetStackChan().leftNeonLight().setColor(r, g, b);
            GetStackChan().rightNeonLight().setColor(r, g, b);
        });

        _buttons.push_back(std::move(btn));
    }

    auto btn_quit = std::make_unique<Button>(*_panel);
    apply_button_common_style(*btn_quit);
    btn_quit->setSize(200, 50);
    btn_quit->label().setText("返回");
    btn_quit->onClick().connect([this]() { _is_done = true; });
    _buttons.push_back(std::move(btn_quit));
}

RgbTestWorker::~RgbTestWorker()
{
    GetStackChan().leftNeonLight().setColor(0, 0, 0);
    GetStackChan().rightNeonLight().setColor(0, 0, 0);
}
