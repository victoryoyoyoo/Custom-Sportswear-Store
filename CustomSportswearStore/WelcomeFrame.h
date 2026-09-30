#pragma once
#include <wx/wx.h>

// Landing window: the store name and a way in, next to a jersey and a ball
// turning in 3D.
class WelcomeFrame : public wxFrame {
public:
    explicit WelcomeFrame(const wxString& title);

private:
    void EnterStore();

    bool m_entering = false;
};
