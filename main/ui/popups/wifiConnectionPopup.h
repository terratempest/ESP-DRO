#pragma once

#include <functional>

class WifiConnectionPopup {
public:
    static void show(std::function<void()> onClose = nullptr);
};
