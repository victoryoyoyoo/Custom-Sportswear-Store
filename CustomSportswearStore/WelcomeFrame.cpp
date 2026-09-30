#include "WelcomeFrame.h"
#include "LauncherFrame.h"
#include "Theme.h"

WelcomeFrame::WelcomeFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title) {
    SetIcon(wxICON(aaaa_app));
    Theme::InstallFullScreenKeys(this);

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(Theme::kNavy);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Title, tagline and product showcase are one piece of artwork; it scales
    // with the window and its edges fade into the background colour, so it
    // looks the same windowed or full screen. Clicking it also enters.
    auto* banner = Theme::ImagePanel::ForAsset(panel, wxT("banner.png"), wxSize(900, 400));
    banner->SetCursor(wxCursor(wxCURSOR_HAND));
    sizer->Add(banner, 1, wxEXPAND);

    auto* enter = Theme::MakePrimaryButton(panel, wxT("進入商店  →"), 13);
    enter->SetMinSize(FromDIP(wxSize(220, 50)));
    sizer->Add(enter, 0, wxALIGN_CENTER | wxTOP, FromDIP(4));
    sizer->Add(Theme::MakeLabel(panel, wxT("8 類商品・12 款配色・可客製姓名與背號・滿 NT$2,000 免運・F11 全螢幕"),
                                10, false, Theme::kOnDarkMuted),
               0, wxALIGN_CENTER | wxTOP | wxBOTTOM, FromDIP(16));

    panel->SetSizer(sizer);
    Theme::FitFrameToContent(this, panel, wxSize(900, 520));
    SetMinClientSize(FromDIP(wxSize(720, 460)));
    Centre();

    enter->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EnterStore(); });
    banner->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) { EnterStore(); });
    enter->SetFocus();  // Enter / Space works right away
}

void WelcomeFrame::EnterStore() {
    if (m_entering) return;
    m_entering = true;
    Theme::ShowLike(new LauncherFrame(), this, [this] { Close(); });
}
