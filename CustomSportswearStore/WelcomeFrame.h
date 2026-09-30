#pragma once
#include <wx/wx.h>

// Landing window: brand artwork + "enter store" button.
class WelcomeFrame : public wxFrame {
public:
    explicit WelcomeFrame(const wxString& title);

private:
    void EnterStore();

    bool m_entering = false;
};
