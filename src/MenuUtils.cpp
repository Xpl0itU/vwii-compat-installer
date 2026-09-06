#include "MenuUtils.h"
#include "ScreenUtils.h"
#include "StateUtils.h"
#include "InputUtils.h"
#include "log.h"
#include <unistd.h>

static void DrawMenu(const std::vector<std::string>& header, const std::vector<std::string>& options, int selected, bool allowCancel) {
    ScreenUtils_ClearBuffer(0);
    int header_lines = std::min((int)header.size(), 14);
    for (int i = 0; i < header_lines; i++) {
        ScreenUtils_PutFont(0, i, header[i].c_str());
    }

    bool has_gap = (header_lines + 2 < 17);
    int opt_start = header_lines + (has_gap ? 1 : 0);
    int max_display = std::max(1, 17 - opt_start);
    int count = (int)options.size();
    int start_idx = std::max(0, std::min(selected - max_display / 2, count - max_display));

    for (int i = 0; i < max_display && start_idx + i < count; i++) {
        int idx = start_idx + i;
        std::string line = (idx == selected ? "-> " : "   ") + options[idx];
        ScreenUtils_PutFont(0, opt_start + i, line.c_str());
    }

    if (allowCancel) {
        ScreenUtils_PutFont(0, 17, "A: Select | B: Cancel | D-Pad: Move/Page");
    } else {
        ScreenUtils_PutFont(0, 17, "A: Select | D-Pad: Move/Page | HOME: Exit");
    }
    ScreenUtils_FlipBuffers();
}

int ShowMenu(const std::vector<std::string>& header, const std::vector<std::string>& options, int defaultSelected, bool allowCancel) {
    if (options.empty()) return -1;
    
    int selected = std::max(0, std::min(defaultSelected, (int)options.size() - 1));
    auto Draw = [&]() {
        DrawMenu(header, options, selected, allowCancel);
    };
    
    Draw();
    
    Input input;
    int hold_timer_up = 0, hold_timer_down = 0;
    int hold_timer_left = 0, hold_timer_right = 0;

    while (State::AppRunning()) {
        input.read();
        
        bool changed = false;
        if (input.getHoldRepeat(PAD_BUTTON_UP, hold_timer_up) && selected > 0) {
            selected--;
            changed = true;
        }
        else if (input.getHoldRepeat(PAD_BUTTON_DOWN, hold_timer_down) && selected < (int)options.size() - 1) {
            selected++;
            changed = true;
        }
        else if (input.getHoldRepeat(PAD_BUTTON_LEFT, hold_timer_left) && selected > 0) {
            selected = std::max(0, selected - 10);
            changed = true;
        }
        else if (input.getHoldRepeat(PAD_BUTTON_RIGHT, hold_timer_right) && selected < (int)options.size() - 1) {
            selected = std::min((int)options.size() - 1, selected + 10);
            changed = true;
        }
        else if (allowCancel && input.get(TRIGGER, PAD_BUTTON_B)) {
            return -1;
        }
        else if (input.get(TRIGGER, PAD_BUTTON_A)) {
            return selected;
        }
        
        if (changed) Draw();
        usleep(16000);
    }
    return -1;
}

static void DrawMultiSelectMenu(const std::vector<std::string>& header, const std::vector<std::string>& options, const std::vector<bool>& selectedOptions, int cursor) {
    ScreenUtils_ClearBuffer(0);
    int header_lines = std::min((int)header.size(), 14);
    for (int i = 0; i < header_lines; i++) {
        ScreenUtils_PutFont(0, i, header[i].c_str());
    }

    bool has_gap = (header_lines + 2 < 17);
    int opt_start = header_lines + (has_gap ? 1 : 0);
    int max_display = std::max(1, 17 - opt_start);
    int count = (int)options.size();
    int start_idx = std::max(0, std::min(cursor - max_display / 2, count - max_display));

    for (int i = 0; i < max_display && start_idx + i < count; i++) {
        int idx = start_idx + i;
        std::string prefix = (idx == cursor ? "-> " : "   ");
        prefix += (selectedOptions[idx] ? "[x] " : "[ ] ");
        std::string line = prefix + options[idx];
        ScreenUtils_PutFont(0, opt_start + i, line.c_str());
    }

    ScreenUtils_PutFont(0, 17, "A: Toggle | X: All | +: Confirm | B: Cancel | D-Pad: Move/Page");
    ScreenUtils_FlipBuffers();
}

std::vector<int> ShowMultiSelectMenu(const std::vector<std::string>& header, const std::vector<std::string>& options, bool defaultSelected) {
    std::vector<int> result;
    if (options.empty()) return result;
    
    int cursor = 0;
    std::vector<bool> selectedOptions(options.size(), defaultSelected);
    
    auto Draw = [&]() {
        DrawMultiSelectMenu(header, options, selectedOptions, cursor);
    };
    
    Draw();
    
    Input input;
    int hold_timer_up = 0, hold_timer_down = 0;
    int hold_timer_left = 0, hold_timer_right = 0;

    while (State::AppRunning()) {
        input.read();
        
        bool changed = false;
        if (input.getHoldRepeat(PAD_BUTTON_UP, hold_timer_up) && cursor > 0) {
            cursor--;
            changed = true;
        }
        else if (input.getHoldRepeat(PAD_BUTTON_DOWN, hold_timer_down) && cursor < (int)options.size() - 1) {
            cursor++;
            changed = true;
        }
        else if (input.getHoldRepeat(PAD_BUTTON_LEFT, hold_timer_left) && cursor > 0) {
            cursor = std::max(0, cursor - 10);
            changed = true;
        }
        else if (input.getHoldRepeat(PAD_BUTTON_RIGHT, hold_timer_right) && cursor < (int)options.size() - 1) {
            cursor = std::min((int)options.size() - 1, cursor + 10);
            changed = true;
        }
        else if (input.get(TRIGGER, PAD_BUTTON_B)) {
            return std::vector<int>(); // Empty result for cancel
        }
        else if (input.get(TRIGGER, PAD_BUTTON_A)) {
            selectedOptions[cursor] = !selectedOptions[cursor];
            changed = true;
        }
        else if (input.get(TRIGGER, PAD_BUTTON_X)) {
            bool anyUnselected = false;
            for (bool isSelected : selectedOptions) {
                if (!isSelected) {
                    anyUnselected = true;
                    break;
                }
            }
            for (size_t i = 0; i < selectedOptions.size(); i++) {
                selectedOptions[i] = anyUnselected;
            }
            changed = true;
        }
        else if (input.get(TRIGGER, PAD_BUTTON_PLUS)) {
            for (size_t i = 0; i < selectedOptions.size(); i++) {
                if (selectedOptions[i]) {
                    result.push_back((int)i);
                }
            }
            if (result.empty()) {
                result.push_back(cursor);
            }
            WUPI_resetScreen();
            return result;
        }
        
        if (changed) {
            Draw();
        }
        
        usleep(16000);
    }
    return result;
}

bool WaitPrompt() {
    Input input;
    while (State::AppRunning()) {
        input.read();
        if (input.get(TRIGGER, PAD_BUTTON_A)) return true;
        if (input.get(TRIGGER, PAD_BUTTON_B)) return false;
        usleep(16000);
    }
    return false;
}
