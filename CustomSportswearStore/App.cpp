#include <wx/wx.h>
#include "WelcomeFrame.h"
#include "Widgets.h"

class StoreApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) return false;
        wxInitAllImageHandlers();
        SetAppDisplayName(wxT("運動用品客製購物系統"));
        WelcomeFrame* frame = new WelcomeFrame(wxT("運動用品客製購物系統 Custom Sportswear Store"));
        Widgets::FadeIn(frame, [frame] { frame->Show(); });
        return true;
    }
};

wxIMPLEMENT_APP(StoreApp);
