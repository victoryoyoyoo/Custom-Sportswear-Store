#include <wx/wx.h>
#include "WelcomeFrame.h"
#include "Lang.h"
#include "Widgets.h"

class StoreApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) return false;
        SetAppName(wxT("CustomSportswearStore"));
        wxInitAllImageHandlers();
        SetAppDisplayName(L(wxT("運動用品客製購物系統"), wxT("Custom Sportswear Store")));
        WelcomeFrame* frame = new WelcomeFrame();
        Widgets::FadeIn(frame, [frame] { frame->Show(); });
        return true;
    }
};

wxIMPLEMENT_APP(StoreApp);
