#pragma once
#include <wx/wx.h>

class WelcomeFrame : public wxFrame {
public:
    WelcomeFrame();

private:
    void EnterStore();
    void SwitchLanguage();

    bool m_entering = false;
};
