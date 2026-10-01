#pragma once
#include <wx/wx.h>

// Landing window: the store name and a way in, next to a jersey and a ball
// turning in 3D.
class WelcomeFrame : public wxFrame {
public:
    WelcomeFrame();

private:
    void EnterStore();
    void SwitchLanguage();

    bool m_entering = false;
};
