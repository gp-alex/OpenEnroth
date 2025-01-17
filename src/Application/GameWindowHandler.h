#pragma once
#include <memory>
#include <tuple>

#include "Platform/PlatformKey.h"
#include "Platform/PlatformEventHandler.h"
#include "Io/Mouse.h"
#include "Utility/Geometry/Size.h"

#include "GameKeyboardController.h"

using Io::Mouse;

namespace Application {
class GameConfig;

// Handles events from game window (OSWindow)
class GameWindowHandler : public PlatformEventHandler {
 public:
    GameWindowHandler();

    GameKeyboardController *KeyboardController() const {
        return keyboardController_.get();
    }

    // TODO(captainurist): this probably doesn't even belong here. Find a place to move to.
    void UpdateWindowFromConfig(const GameConfig *config);
    void UpdateConfigFromWindow(GameConfig *config);
    std::tuple<int, Pointi, Sizei> GetWindowConfigPosition(const GameConfig *config);
    std::tuple<int, Pointi, Sizei> GetWindowRelativePosition(Pointi *position = nullptr);

 private:
    void OnMouseLeftClick(int x, int y);
    void OnMouseRightClick(int x, int y);
    void OnMouseLeftUp();
    void OnMouseRightUp();
    void OnMouseLeftDoubleClick(int x, int y);
    void OnMouseRightDoubleClick(int x, int y);
    void OnMouseMove(int x, int y, bool left_button, bool right_button);
    void OnScreenshot();
    void OnToggleBorderless();
    void OnToggleFullscreen();
    void OnMouseGrabToggle();
    void OnKey(PlatformKey key);
    bool OnChar(PlatformKey key, int c);
    void OnFocus();
    void OnFocusLost();
    void OnPaint();
    void OnActivated();
    void OnDeactivated();

    virtual void Event(PlatformWindow *window, const PlatformEvent *event) override;
    virtual void KeyPressEvent(PlatformWindow *window, const PlatformKeyEvent *event) override;
    virtual void KeyReleaseEvent(PlatformWindow *window, const PlatformKeyEvent *event) override;
    virtual void MouseMoveEvent(PlatformWindow *window, const PlatformMouseEvent *event) override;
    virtual void MousePressEvent(PlatformWindow *window, const PlatformMouseEvent *event) override;
    virtual void MouseReleaseEvent(PlatformWindow *window, const PlatformMouseEvent *event) override;
    virtual void WheelEvent(PlatformWindow *window, const PlatformWheelEvent *event) override;
    virtual void MoveEvent(PlatformWindow *window, const PlatformMoveEvent *event) override;
    virtual void ActivationEvent(PlatformWindow *window, const PlatformEvent *event) override;
    virtual void CloseEvent(PlatformWindow *window, const PlatformEvent *event) override;

 private:
    std::shared_ptr<Mouse> mouse = nullptr;
    std::unique_ptr<GameKeyboardController> keyboardController_;
};

}  // namespace Application
