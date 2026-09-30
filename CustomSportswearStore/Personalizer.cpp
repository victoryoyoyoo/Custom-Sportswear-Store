#include "Personalizer.h"
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

    // Heat-press look: the outline colour is stamped in a small circle, then
    // the fill goes on top.
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

    // Largest font (up to the area's height) whose text fits the area's width.
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

    void BindChange(wxSpinCtrl* spin, const std::function<void()>& onChange) {
        spin->Bind(wxEVT_SPINCTRL, [onChange](wxSpinEvent&) { onChange(); });
        spin->Bind(wxEVT_TEXT, [onChange](wxCommandEvent&) { onChange(); });
    }
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

// ---------------------------------------------------------------------------
// Name + number (jersey)
// ---------------------------------------------------------------------------
void NameAndNumberPersonalizer::BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void()> onChange) {
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 2, parent->FromDIP(6), parent->FromDIP(16));
    grid->Add(Theme::MakeLabel(parent, wxT("背號（0–99）"), 10, false, Theme::kMuted));
    grid->Add(Theme::MakeLabel(parent, wxString::Format(wxT("印製姓名（選填，最多 %d 字）"), m_product.maxTextLength),
                               10, false, Theme::kMuted));
    m_number = MakeNumberSpin(parent, 23);
    m_name = MakeText(parent, m_product.maxTextLength, wxT("例如：WANG"));
    grid->Add(m_number);
    grid->Add(m_name, 1, wxEXPAND);
    grid->AddGrowableCol(1, 1);
    sizer->Add(grid, 0, wxEXPAND);
    sizer->AddSpacer(parent->FromDIP(8));
    sizer->Add(Theme::MakeLabel(parent, wxT("隊名（印在正面，選填）"), 10, false, Theme::kMuted), 0, wxBOTTOM, parent->FromDIP(6));
    m_team = MakeText(parent, 14, wxT("例如：TIGERS"));
    sizer->Add(m_team, 0, wxEXPAND);

    BindChange(m_number, onChange);
    m_name->Bind(wxEVT_TEXT, [onChange](wxCommandEvent&) { onChange(); });
    m_team->Bind(wxEVT_TEXT, [onChange](wxCommandEvent&) { onChange(); });
}

wxString NameAndNumberPersonalizer::Spec(int number, const wxString& name, const wxString& team) {
    wxString s = wxString::Format(wxT("#%d"), number);
    if (!name.IsEmpty()) s += wxT("・") + name;
    if (!team.IsEmpty()) s += wxT("・正面 ") + team;
    return s;
}

wxString NameAndNumberPersonalizer::TeamName() const { return Cleaned(m_team, true); }

wxString NameAndNumberPersonalizer::PrintedName() const { return Cleaned(m_name, true); }

wxString NameAndNumberPersonalizer::Describe() const {
    return Spec(m_number->GetValue(), PrintedName(), TeamName());
}

void NameAndNumberPersonalizer::Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& c, bool front) const {
    const double k = art.m_width / m_product.artWidth;
    if (front) {
        // Team name across the chest, a smaller number under it.
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

// ---------------------------------------------------------------------------
// Number only (shorts)
// ---------------------------------------------------------------------------
void NumberPersonalizer::BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void()> onChange) {
    sizer->Add(Theme::MakeLabel(parent, wxT("背號（0–99）"), 10, false, Theme::kMuted), 0, wxBOTTOM, parent->FromDIP(6));
    m_number = MakeNumberSpin(parent, 23);
    sizer->Add(m_number);
    BindChange(m_number, onChange);
}

wxString NumberPersonalizer::Describe() const {
    return wxString::Format(wxT("#%d"), m_number->GetValue());
}

void NumberPersonalizer::Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& c, bool) const {
    const double k = art.m_width / m_product.artWidth;
    const PrintArea& a = m_product.numberArea;
    wxFont font(wxFontInfo(wxSize(0, (int)(a.height * k))).FaceName(wxT("Impact")));
    DrawOutlinedText(gc, wxString::Format(wxT("%d"), m_number->GetValue()), font,
                     art.m_x + a.centerX * k, art.m_y + a.top * k, wxColour(255, 255, 255), c.trim, 3 * k);
}

// ---------------------------------------------------------------------------
// Short text (cap, ball, wristband, backpack)
// ---------------------------------------------------------------------------
void TextPersonalizer::BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void()> onChange) {
    sizer->Add(Theme::MakeLabel(parent, wxString::Format(wxT("最多 %d 字，中英文皆可"), m_product.maxTextLength),
                                10, false, Theme::kMuted),
               0, wxBOTTOM, parent->FromDIP(6));
    m_text = MakeText(parent, m_product.maxTextLength, wxT("例如：TEAM WANG"));
    sizer->Add(m_text, 0, wxEXPAND);
    m_text->Bind(wxEVT_TEXT, [onChange](wxCommandEvent&) { onChange(); });
}

wxString TextPersonalizer::Text() const { return Cleaned(m_text, false); }

wxString TextPersonalizer::Describe() const {
    return Text().IsEmpty() ? wxString() : wxT("「") + Text() + wxT("」");
}

void TextPersonalizer::Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& c, bool) const {
    const wxString text = Text();
    if (text.IsEmpty()) return;
    const double k = art.m_width / m_product.artWidth;

    if (m_product.textArea.IsSet()) {
        const PrintArea& a = m_product.textArea;
        wxFont font = FitFont(gc, text, NameFace(text), true, a.maxWidth * k, a.height * k);
        const double shrink = a.height * k - font.GetPixelSize().GetHeight();
        // Printed in the fabric colour on the trim-coloured pocket / panel.
        const bool onTrim = m_product.id == wxT("backpack");
        DrawOutlinedText(gc, text, font, art.m_x + a.centerX * k, art.m_y + a.top * k + shrink / 2,
                         onTrim ? c.fabric : c.trim, onTrim ? c.trim : c.fabric, 1.5 * k);
        return;
    }

    // No room on the artwork: a stitched label tucked into the bottom-left corner.
    const double pad = 10 * k + 6;
    wxFont font = FitFont(gc, text, NameFace(text), true, art.m_width * 0.55, 30 * k + 8);
    wxDouble tw, th;
    gc->SetFont(font, c.trim);
    gc->GetTextExtent(text, &tw, &th);
    const double w = tw + pad * 2, h = th + pad * 1.4;
    const double x = art.m_x + 4, y = art.m_y + art.m_height - h - 4;
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(c.fabric));
    gc->DrawRoundedRectangle(x, y, w, h, h / 2);
    wxPen stitch(c.trim, std::max(1, (int)(1.2 * k + 0.5)), wxPENSTYLE_SHORT_DASH);
    gc->SetPen(stitch);
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    const double inset = pad * 0.35;
    gc->DrawRoundedRectangle(x + inset, y + inset, w - 2 * inset, h - 2 * inset, (h - 2 * inset) / 2);
    gc->SetFont(font, c.trim);
    gc->DrawText(text, x + pad, y + (h - th) / 2);

    // Small caption so it's clear this is where the stitching goes.
    wxFont caption(wxFontInfo(wxSize(0, std::max(10, (int)(th * 0.42)))).FaceName(wxT("Microsoft JhengHei UI")));
    wxDouble cw, ch;
    gc->SetFont(caption, wxColour(110, 120, 138));
    gc->GetTextExtent(wxT("刺繡預覽"), &cw, &ch);
    gc->DrawText(wxT("刺繡預覽"), x + pad * 0.6, y - ch - 2);
}
