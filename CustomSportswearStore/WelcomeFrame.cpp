#include "WelcomeFrame.h"
#include "Lang.h"
#include "LauncherFrame.h"
#include "Personalizer.h"
#include "Showcase.h"
#include "Theme.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <algorithm>
#include <cmath>

namespace {
    const Product* FindProduct(const wxString& id) {
        for (const Product& p : Catalog::Products())
            if (p.id == id) return &p;
        return nullptr;
    }

    const Colorway& FindColorway(const wxString& id) {
        for (const Colorway& c : Catalog::Colorways())
            if (c.id == id) return c;
        return Catalog::Colorways().front();
    }

    wxWindow* MakePill(wxWindow* parent, const wxString& text) {
        auto* pill = new wxWindow(parent, wxID_ANY);
        pill->SetBackgroundStyle(wxBG_STYLE_PAINT);
        const wxFont font = Theme::Font(10, true);
        wxClientDC measure(pill);
        measure.SetFont(font);
        const wxSize t = measure.GetTextExtent(text);
        pill->SetMinSize(wxSize(t.x + pill->FromDIP(26), t.y + pill->FromDIP(12)));
        pill->Bind(wxEVT_PAINT, [pill, text, font](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(pill);
            dc.SetBackground(wxBrush(pill->GetParent()->GetBackgroundColour()));
            dc.Clear();
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (!gc) return;
            const wxSize s = pill->GetClientSize();
            gc->SetPen(wxPen(wxColour(96, 112, 142), 1));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawRoundedRectangle(0.5, 0.5, s.x - 1, s.y - 1, (s.y - 1) / 2.0);
            wxDouble tw, th;
            gc->SetFont(font, wxColour(198, 208, 226));
            gc->GetTextExtent(text, &tw, &th);
            gc->DrawText(text, (s.x - tw) / 2, (s.y - th) / 2);
        });
        return pill;
    }

    class Stage : public wxWindow {
    public:
        explicit Stage(wxWindow* parent) : wxWindow(parent, wxID_ANY) {
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            SetMinSize(FromDIP(wxSize(440, 360)));
            Bind(wxEVT_PAINT, &Stage::OnPaint, this);
            Bind(wxEVT_SIZE, [this](wxSizeEvent& event) { m_stale = true; Refresh(false); event.Skip(); });
            m_timer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { Refresh(false); });
            m_startMs = wxGetLocalTimeMillis();
            if (const Product* p = FindProduct(wxT("jersey"))) {
                m_print = Personalizer::For(*p);
                auto* hidden = new wxPanel(this);
                hidden->Hide();
                auto* sizer = new wxBoxSizer(wxVERTICAL);
                m_print->BuildControls(hidden, sizer, [](int) {});
                hidden->SetSizer(sizer);
            }
        }

    private:
        void Build() {
            m_stale = false;
            const wxSize size = GetClientSize();
            m_jerseySize = wxSize((int)(size.x * 0.78), (int)(size.y * 0.92));
            m_ballSize = wxSize((int)(size.y * 0.42), (int)(size.y * 0.42));
            m_jersey.reset();
            m_ball.reset();
            if (m_jerseySize.x < 16 || m_ballSize.x < 16) return;
            if (const Product* p = FindProduct(wxT("jersey")))
                m_jersey = Showcase::Build(*p, FindColorway(wxT("crimson")), m_print.get(), m_jerseySize);
            if (const Product* p = FindProduct(wxT("basketball")))
                m_ball = Showcase::Build(*p, FindColorway(wxT("sunset")), nullptr, m_ballSize);
        }

        void OnPaint(wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.SetBackground(wxBrush(Theme::kNavy));
            dc.Clear();
            const wxSize size = GetClientSize();
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (!gc || size.x < 16 || size.y < 16) return;
            if (m_stale) Build();
            const wxLongLong started = wxGetLocalTimeMillis();

            {
                wxGraphicsGradientStops glow(wxColour(58, 74, 108, 150), wxColour(14, 23, 42, 0));
                gc->SetBrush(gc->CreateRadialGradientBrush(size.x * 0.52, size.y * 0.45, size.x * 0.52, size.y * 0.45,
                                                           std::min(size.x, size.y) * 0.55, glow));
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRectangle(0, 0, size.x, size.y);
                gc->PushState();
                gc->Translate(size.x * 0.52, size.y * 0.90);
                gc->Scale(1.0, 0.16);
                wxGraphicsGradientStops floor(wxColour(90, 110, 150, 70), wxColour(14, 23, 42, 0));
                gc->SetBrush(gc->CreateRadialGradientBrush(0, 0, 0, 0, size.x * 0.42, floor));
                gc->DrawEllipse(-size.x * 0.42, -size.x * 0.42, size.x * 0.84, size.x * 0.84);
                gc->PopState();
            }

            const double seconds = (wxGetLocalTimeMillis() - m_startMs).ToDouble() / 1000.0;
            const double sway = 0.55 * std::sin(seconds * 2 * 3.14159265358979 / 9.0);
            gc->SetInterpolationQuality(wxINTERPOLATION_GOOD);
            const double q = m_quality;
            const wxSize jerseyPixels((int)(m_jerseySize.x * q), (int)(m_jerseySize.y * q));
            const wxSize ballPixels((int)(m_ballSize.x * q), (int)(m_ballSize.y * q));
            if (m_jersey) {
                Showcase::Frame f = m_jersey->Render(sway, jerseyPixels, true);
                f.bounds = wxRect((int)(f.bounds.x / q), (int)(f.bounds.y / q), (int)(f.bounds.width / q), (int)(f.bounds.height / q));
                const double x = (size.x - m_jerseySize.x) * 0.40, y = (size.y - m_jerseySize.y) * 0.25;
                wxRect shadow = f.bounds;
                shadow.Offset((int)x, (int)y);
                Showcase::DrawShadow(gc.get(), shadow, 1.6);
                gc->DrawBitmap(wxBitmap(f.image), x, y, m_jerseySize.x, m_jerseySize.y);
            }
            if (m_ball) {
                Showcase::Frame f = m_ball->Render(-seconds * 2 * 3.14159265358979 / 9.0, ballPixels, true);
                f.bounds = wxRect((int)(f.bounds.x / q), (int)(f.bounds.y / q), (int)(f.bounds.width / q), (int)(f.bounds.height / q));
                const double x = size.x - m_ballSize.x * 1.05, y = size.y - m_ballSize.y * 1.02;
                wxRect shadow = f.bounds;
                shadow.Offset((int)x, (int)y);
                Showcase::DrawShadow(gc.get(), shadow, 1.8);
                gc->DrawBitmap(wxBitmap(f.image), x, y, m_ballSize.x, m_ballSize.y);
            }
            const double took = (wxGetLocalTimeMillis() - started).ToDouble();
            if (took > 28 && m_quality > 0.45) m_quality -= 0.1;
            else if (took < 12 && m_quality < 1.0) m_quality = std::min(1.0, m_quality + 0.05);
            m_timer.StartOnce(std::max(16, 33 - (int)took));
        }

        std::unique_ptr<Personalizer> m_print;
        std::unique_ptr<Showcase::Model> m_jersey, m_ball;
        wxSize m_jerseySize, m_ballSize;
        bool m_stale = true;
        double m_quality = 1.0;
        wxTimer m_timer;
        wxLongLong m_startMs;
    };
}

WelcomeFrame::WelcomeFrame()
    : wxFrame(nullptr, wxID_ANY, L(wxT("運動用品客製購物系統 Custom Sportswear Store"), wxT("Custom Sportswear Store"))) {
    SetIcon(wxICON(aaaa_app));
    Theme::InstallFullScreenKeys(this);

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(Theme::kNavy);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);

    auto* language = new Widgets::FlatButton(panel, Lang::English() ? wxString(wxT("中文")) : wxString(wxT("English")),
                                             Widgets::FlatButton::Style::OnDark, 10, 34);
    language->SetMinSize(FromDIP(wxSize(96, 34)));
    root->Add(language, 0, wxALIGN_RIGHT | wxTOP | wxRIGHT, FromDIP(14));

    wxBoxSizer* body = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* text = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* titleRow = new wxBoxSizer(wxHORIZONTAL);
    wxPanel* bar = new wxPanel(panel, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(5, 64)));
    bar->SetBackgroundColour(Theme::kOrange);
    titleRow->Add(bar, 0, wxEXPAND | wxRIGHT, FromDIP(16));
    wxBoxSizer* names = new wxBoxSizer(wxVERTICAL);
    names->Add(Theme::MakeLabel(panel, L(wxT("運動用品客製購物系統"), wxT("Custom Sportswear Store")), 26, true, Theme::kOnDark));
    names->Add(Theme::MakeLabel(panel, L(wxT("C U S T O M   S P O R T S W E A R   S T O R E"),
                                                 wxT("T E A M W E A R   ·   M A D E   T O   O R D E R")), 9, true, Theme::kOnDarkMuted),
               0, wxTOP, FromDIP(6));
    titleRow->Add(names, 0, wxALIGN_CENTER_VERTICAL);
    text->Add(titleRow);

    text->Add(Theme::MakeLabel(panel, wxString::Format(L(wxT("%zu 項商品・%zu 款配色・360° 即時預覽"), wxT("%zu products · %zu colourways · live 360° preview")),
                                                       Catalog::Products().size(), Catalog::Colorways().size()),
                               13, false, wxColour(222, 228, 240)),
              0, wxTOP, FromDIP(34));
    text->Add(Theme::MakeLabel(panel, L(wxT("球衣、球鞋到水壺，印上名字前先轉一圈看清楚。"), wxT("From jerseys to bottles: turn it round before your name goes on.")), 11, false, Theme::kOnDarkMuted),
              0, wxTOP, FromDIP(8));

    wxBoxSizer* pills = new wxBoxSizer(wxHORIZONTAL);
    for (const wxString& label : { L(wxT("360° 預覽"), wxT("360° view")), L(wxT("客製印字"), wxT("Custom print")), L(wxT("團體訂購"), wxT("Team order")), L(wxT("滿額免運"), wxT("Free shipping")) })
        pills->Add(MakePill(panel, label), 0, wxRIGHT, FromDIP(8));
    text->Add(pills, 0, wxTOP, FromDIP(22));

    auto* enter = Theme::MakePrimaryButton(panel, L(wxT("進入商店"), wxT("Enter the store")), 13);
    enter->ShowArrow();
    enter->SetMinSize(FromDIP(wxSize(230, 52)));
    text->Add(enter, 0, wxTOP, FromDIP(36));

    body->Add(text, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(56));
    body->AddSpacer(FromDIP(24));
    auto* stage = new Stage(panel);
    stage->SetCursor(wxCursor(wxCURSOR_HAND));
    body->Add(stage, 1, wxEXPAND | wxTOP | wxRIGHT, FromDIP(12));
    root->Add(body, 1, wxEXPAND);

    root->Add(Theme::MakeLabel(panel, wxString::Format(L(wxT("單筆滿 %s 免運・F11 全螢幕"), wxT("Free shipping over %s · F11 full screen")),
                                                       Theme::FormatPrice(Catalog::kFreeShippingThreshold)),
                               9, false, wxColour(112, 126, 152)),
              0, wxALIGN_CENTER | wxTOP | wxBOTTOM, FromDIP(14));

    panel->SetSizer(root);
    Theme::FitFrameToContent(this, panel, wxSize(1000, 560));
    SetMinClientSize(FromDIP(wxSize(900, 500)));
    Centre();

    language->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SwitchLanguage(); });
    enter->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EnterStore(); });
    stage->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) { EnterStore(); });
    enter->SetFocus();
}

void WelcomeFrame::SwitchLanguage() {
    if (m_entering) return;
    m_entering = true;
    Lang::SetEnglish(!Lang::English());
    Theme::ShowLike(new WelcomeFrame(), this, [this] { Destroy(); });
}

void WelcomeFrame::EnterStore() {
    if (m_entering) return;
    m_entering = true;
    Theme::ShowLike(new LauncherFrame(), this, [this] { Close(); });
}
