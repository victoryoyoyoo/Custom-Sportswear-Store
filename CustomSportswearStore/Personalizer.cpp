#include "Personalizer.h"
#include "Lang.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>

namespace {
    bool IsAscii(const wxString& s) {
        return std::all_of(s.begin(), s.end(), [](wxUniChar c) { return c.IsAscii(); });
    }

    wxString Cleaned(const wxTextCtrl* ctrl, bool upper) {
        wxString s = ctrl ? ctrl->GetValue() : wxString();
        s.Trim().Trim(false);
        return upper ? s.Upper() : s;
    }

    void DrawOutlinedText(wxGraphicsContext* gc, const wxString& text, const wxFont& font, double centerX,
                          double top, const wxColour& fill, const wxColour& outline, double stroke) {
        wxDouble w, h;
        gc->SetFont(font, outline);
        gc->GetTextExtent(text, &w, &h);
        const double x = centerX - w / 2;
        for (int i = 0; i < 16; ++i) {
            const double a = i * 3.14159265358979 / 8;
            gc->DrawText(text, x + stroke * std::cos(a), top + stroke * std::sin(a));
        }
        gc->SetFont(font, fill);
        gc->DrawText(text, x, top);
    }

    wxFont FitFont(wxGraphicsContext* gc, const wxString& text, const wxString& face, bool bold,
                   double maxWidth, double height) {
        int px = std::max(8, (int)height);
        wxFont font;
        wxDouble w = 0, h = 0;
        do {
            wxFontInfo info(wxSize(0, px));
            info.FaceName(face);
            if (bold) info.Bold();
            font = wxFont(info);
            gc->SetFont(font, *wxBLACK);
            gc->GetTextExtent(text, &w, &h);
        } while (w > maxWidth && --px > 8);
        return font;
    }

    wxString NameFace(const wxString& s) { return IsAscii(s) ? wxT("Bahnschrift") : wxT("Microsoft JhengHei UI"); }

    wxSpinCtrl* MakeNumberSpin(wxWindow* parent, int initial) {
        auto* spin = new wxSpinCtrl(parent, wxID_ANY, wxString::Format(wxT("%d"), initial), wxDefaultPosition,
                                    parent->FromDIP(wxSize(110, -1)), wxSP_ARROW_KEYS, 0, 99, initial);
        spin->SetFont(Theme::Font(11));
        return spin;
    }

    wxTextCtrl* MakeText(wxWindow* parent, int maxLength, const wxString& hint) {
        auto* text = new wxTextCtrl(parent, wxID_ANY);
        text->SetFont(Theme::Font(11));
        text->SetMaxLength(maxLength);
        text->SetHint(hint);
        return text;
    }

    void BindChange(wxSpinCtrl* spin, const std::function<void(int)>& onChange, int side) {
        spin->Bind(wxEVT_SPINCTRL, [onChange, side](wxSpinEvent&) { onChange(side); });
        spin->Bind(wxEVT_TEXT, [onChange, side](wxCommandEvent&) { onChange(side); });
    }

    void BindChange(wxTextCtrl* text, const std::function<void(int)>& onChange, int side) {
        text->Bind(wxEVT_TEXT, [onChange, side](wxCommandEvent&) { onChange(side); });
    }
}

wxImage Personalizer::TextDecal(const wxString& text, const wxSize& pixels, const wxColour& fill, const wxColour& outline) {
    wxImage image(pixels.x, pixels.y);
    image.InitAlpha();
    memset(image.GetAlpha(), 0, (size_t)pixels.x * pixels.y);
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(image));
    if (gc && !text.IsEmpty()) {
        const double stroke = std::max(1.0, pixels.y * 0.05);
        wxFont font = FitFont(gc.get(), text, NameFace(text), true, pixels.x - 4 * stroke, pixels.y - 2 * stroke);
        const double top = (pixels.y - font.GetPixelSize().GetHeight()) / 2.0;
        DrawOutlinedText(gc.get(), text, font, pixels.x / 2.0, top, fill, outline, stroke);
    }
    gc.reset();
    return image;
}

std::unique_ptr<Personalizer> Personalizer::For(const Product& product) {
    switch (product.personalization) {
    case Personalization::NameAndNumber: return std::make_unique<NameAndNumberPersonalizer>(product);
    case Personalization::Number:        return std::make_unique<NumberPersonalizer>(product);
    case Personalization::Text:          return std::make_unique<TextPersonalizer>(product);
    case Personalization::None:          break;
    }
    return std::make_unique<Personalizer>(product);
}

void NameAndNumberPersonalizer::BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int)> onChange) {
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 2, parent->FromDIP(6), parent->FromDIP(16));
    grid->Add(Theme::MakeLabel(parent, L(wxT("背號（0–99）"), wxT("Number (0–99)")), 10, false, Theme::kMuted));
    grid->Add(Theme::MakeLabel(parent, wxString::Format(L(wxT("印製姓名（選填，最多 %d 字）"), wxT("Name (optional, up to %d characters)")), m_product.maxTextLength),
                               10, false, Theme::kMuted));
    m_number = MakeNumberSpin(parent, 23);
    m_name = MakeText(parent, m_product.maxTextLength, L(wxT("例如：WANG"), wxT("e.g. WANG")));
    grid->Add(m_number);
    grid->Add(m_name, 1, wxEXPAND);
    grid->AddGrowableCol(1, 1);
    sizer->Add(grid, 0, wxEXPAND);
    sizer->AddSpacer(parent->FromDIP(8));
    sizer->Add(Theme::MakeLabel(parent, L(wxT("隊名（印在正面，選填）"), wxT("Team name (printed on the front, optional)")), 10, false, Theme::kMuted), 0, wxBOTTOM, parent->FromDIP(6));
    m_team = MakeText(parent, 14, L(wxT("例如：TIGERS"), wxT("e.g. TIGERS")));
    sizer->Add(m_team, 0, wxEXPAND);

    BindChange(m_number, onChange, 0);
    BindChange(m_name, onChange, 0);
    BindChange(m_team, onChange, 1);
}

wxString NameAndNumberPersonalizer::Spec(int number, const wxString& name, const wxString& team) {
    wxString s = wxString::Format(wxT("#%d"), number);
    if (!name.IsEmpty()) s += L(wxT("・"), wxT(" · ")) + name;
    if (!team.IsEmpty()) s += L(wxT("・正面 "), wxT(" · front ")) + team;
    return s;
}

wxString NameAndNumberPersonalizer::TeamName() const { return Cleaned(m_team, true); }

wxString NameAndNumberPersonalizer::PrintedName() const { return Cleaned(m_name, true); }

wxString NameAndNumberPersonalizer::Describe() const {
    return Spec(m_number->GetValue(), PrintedName(), TeamName());
}

void NameAndNumberPersonalizer::Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& c, int side) const {
    const double k = art.m_width / m_product.artWidth;
    if (side == 1) {
        const PrintArea& ta = m_product.teamArea;
        const PrintArea& fn = m_product.frontNumberArea;
        const wxString team = TeamName();
        if (!team.IsEmpty()) {
            wxFont teamFont = FitFont(gc, team, NameFace(team), true, ta.maxWidth * k, ta.height * k);
            const double shrink = ta.height * k - teamFont.GetPixelSize().GetHeight();
            DrawOutlinedText(gc, team, teamFont, art.m_x + ta.centerX * k, art.m_y + ta.top * k + shrink / 2,
                             wxColour(255, 255, 255), c.trim, 3 * k);
        }
        wxFont numberFont(wxFontInfo(wxSize(0, (int)(fn.height * k))).FaceName(wxT("Impact")));
        DrawOutlinedText(gc, wxString::Format(wxT("%d"), m_number->GetValue()), numberFont,
                         art.m_x + fn.centerX * k, art.m_y + (team.IsEmpty() ? fn.top - 30 : fn.top) * k,
                         wxColour(255, 255, 255), c.trim, 4 * k);
        return;
    }
    const PrintArea& num = m_product.numberArea;
    const PrintArea& nm = m_product.nameArea;
    const wxColour white(255, 255, 255);

    wxFont numberFont(wxFontInfo(wxSize(0, (int)(num.height * k))).FaceName(wxT("Impact")));
    DrawOutlinedText(gc, wxString::Format(wxT("%d"), m_number->GetValue()), numberFont,
                     art.m_x + num.centerX * k, art.m_y + num.top * k, white, c.trim, 5 * k);

    const wxString name = PrintedName();
    if (name.IsEmpty()) return;
    wxFont nameFont = FitFont(gc, name, NameFace(name), true, nm.maxWidth * k, nm.height * k);
    const double shrink = nm.height * k - nameFont.GetPixelSize().GetHeight();
    DrawOutlinedText(gc, name, nameFont, art.m_x + nm.centerX * k, art.m_y + nm.top * k + shrink / 2,
                     white, c.trim, 2.5 * k);
}

void NumberPersonalizer::BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int)> onChange) {
    sizer->Add(Theme::MakeLabel(parent, L(wxT("背號（0–99）"), wxT("Number (0–99)")), 10, false, Theme::kMuted), 0, wxBOTTOM, parent->FromDIP(6));
    m_number = MakeNumberSpin(parent, 23);
    sizer->Add(m_number);
    BindChange(m_number, onChange, m_product.printSide);
}

wxString NumberPersonalizer::Describe() const {
    return wxString::Format(wxT("#%d"), m_number->GetValue());
}

void NumberPersonalizer::Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& c, int side) const {
    if (side != m_product.printSide) return;
    const double k = art.m_width / m_product.artWidth;
    const PrintArea& a = m_product.numberArea;
    wxFont font(wxFontInfo(wxSize(0, (int)(a.height * k))).FaceName(wxT("Impact")));
    DrawOutlinedText(gc, wxString::Format(wxT("%d"), m_number->GetValue()), font,
                     art.m_x + a.centerX * k, art.m_y + a.top * k, wxColour(255, 255, 255), c.trim, 3 * k);
}

void TextPersonalizer::BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int)> onChange) {
    sizer->Add(Theme::MakeLabel(parent, wxString::Format(L(wxT("最多 %d 字，中英文皆可"), wxT("Up to %d characters")), m_product.maxTextLength),
                                10, false, Theme::kMuted),
               0, wxBOTTOM, parent->FromDIP(6));
    m_text = MakeText(parent, m_product.maxTextLength, L(wxT("例如：TEAM WANG"), wxT("e.g. TEAM WANG")));
    sizer->Add(m_text, 0, wxEXPAND);
    BindChange(m_text, onChange, m_product.printSide);
}

wxString TextPersonalizer::Text() const { return Cleaned(m_text, false); }

wxString TextPersonalizer::Describe() const {
    return Text().IsEmpty() ? wxString() : L(wxT("「"), wxT("\"")) + Text() + L(wxT("」"), wxT("\""));
}

wxString TextPersonalizer::PrintText() const { return Text(); }

void TextPersonalizer::Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& c, int side) const {
    const wxString text = Text();
    if (text.IsEmpty() || !m_product.textArea.IsSet() || side != m_product.printSide) return;
    const double k = art.m_width / m_product.artWidth;
    const PrintArea& a = m_product.textArea;
    wxFont font = FitFont(gc, text, NameFace(text), true, a.maxWidth * k, a.height * k);
    const double shrink = a.height * k - font.GetPixelSize().GetHeight();
    const bool onTrim = m_product.printOnTrim;
    DrawOutlinedText(gc, text, font, art.m_x + a.centerX * k, art.m_y + a.top * k + shrink / 2,
                     onTrim ? c.fabric : c.trim, onTrim ? c.trim : c.fabric, 1.5 * k);
}
