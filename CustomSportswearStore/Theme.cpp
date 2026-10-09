#include "Theme.h"
#include "Lang.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <memory>
#include <algorithm>

namespace Theme {

wxFont Font(int pointSize, bool bold) {
    wxFontInfo info(pointSize);
    info.FaceName(Lang::English() ? wxT("Segoe UI") : wxT("Microsoft JhengHei UI"));
    if (bold) info.Bold();
    return wxFont(info);
}

wxString AssetPath(const wxString& fileName) {
    wxArrayString roots;
    roots.Add(wxGetCwd());
    wxFileName exeDir(wxStandardPaths::Get().GetExecutablePath());
    for (int up = 0; up <= 3; ++up) {
        roots.Add(exeDir.GetPath());
        if (exeDir.GetDirCount() == 0) break;
        exeDir.RemoveLastDir();
    }
    for (const wxString& root : roots) {
        wxFileName candidate(root + wxFILE_SEP_PATH + wxT("assets"), fileName);
        if (candidate.FileExists()) return candidate.GetFullPath();
    }
    return wxT("assets") + wxString(wxFILE_SEP_PATH) + fileName;
}

ImagePanel::ImagePanel(wxWindow* parent, const wxSize& minDipSize, Renderer renderer)
    : wxPanel(parent, wxID_ANY), m_renderer(std::move(renderer)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(FromDIP(minDipSize));
    Bind(wxEVT_PAINT, &ImagePanel::OnPaint, this);
    m_introTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { StartCrossfade(m_introFrom, m_introTo, 380); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        Rerender();
        event.Skip();
    });
}

void ImagePanel::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();
    const wxSize area = GetClientSize();
    if (m_bitmap.IsOk())
        dc.DrawBitmap(m_bitmap, (area.x - m_bitmap.GetWidth()) / 2, (area.y - m_bitmap.GetHeight()) / 2);
}

void ImagePanel::StartCrossfade(const wxImage& from, const wxImage& to, int durationMs) {
    m_fadeFrom = from;
    m_fadeTo = to;
    m_fade.Start(durationMs, [this](double t) {
        wxImage frame(m_fadeTo.GetWidth(), m_fadeTo.GetHeight(), false);
        const unsigned char* a = m_fadeFrom.GetData();
        const unsigned char* b = m_fadeTo.GetData();
        unsigned char* out = frame.GetData();
        const int wt = (int)(t * 256);
        const size_t n = (size_t)frame.GetWidth() * frame.GetHeight() * 3;
        for (size_t i = 0; i < n; ++i) out[i] = (unsigned char)((a[i] * (256 - wt) + b[i] * wt) >> 8);
        m_bitmap = wxBitmap(frame);
        Refresh(false);
    }, [this] {
        m_bitmap = wxBitmap(m_fadeTo);
        m_fadeFrom = m_fadeTo = wxImage();
        Refresh(false);
    });
}

void ImagePanel::PlayIntro(int delayMs) {
    m_introDelay = std::max(0, delayMs);
    Rerender();
}

void ImagePanel::Rerender() {
    if (m_pending) return;
    m_pending = true;
    CallAfter([this] {
        m_pending = false;
        const wxSize area = GetClientSize();
        if (area.x < 8 || area.y < 8 || !m_renderer) return;
        wxBitmap next = m_renderer(area);

        if (m_introDelay >= 0) {
            const int delay = m_introDelay;
            m_introDelay = -1;
            m_fade.Stop();
            wxImage blank(next.GetWidth(), next.GetHeight());
            const wxColour bg = GetParent()->GetBackgroundColour();
            blank.SetRGB(wxRect(0, 0, blank.GetWidth(), blank.GetHeight()), bg.Red(), bg.Green(), bg.Blue());
            m_bitmap = wxBitmap(blank);
            Refresh(false);
            m_introFrom = blank;
            m_introTo = next.ConvertToImage();
            m_introTimer.StartOnce(std::max(1, delay));
            return;
        }
        m_fade.Stop();
        m_bitmap = next;
        Refresh();
    });
}

void LimitWidth(wxWindow* host, wxSizerItem* item, int maxDip, int minMarginDip) {
    host->Bind(wxEVT_SIZE, [host, item, maxDip, minMarginDip](wxSizeEvent& event) {
        const int width = host->GetClientSize().x;
        const int margin = std::max(host->FromDIP(minMarginDip), (width - host->FromDIP(maxDip)) / 2);
        if (item->GetBorder() != margin) item->SetBorder(margin);
        event.Skip();
    });
}

namespace {
    int g_openDialogs = 0;
}

int ShowModalDialog(wxDialog& dialog) {
    ++g_openDialogs;
    const int result = dialog.ShowModal();
    --g_openDialogs;
    return result;
}

bool Confirm(wxWindow* parent, const wxString& title, const wxString& message, const wxString& yes) {
    wxMessageDialog dialog(parent, message, title, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    dialog.SetYesNoLabels(yes, L(wxT("取消"), wxT("Cancel")));
    return ShowModalDialog(dialog) == wxID_YES;
}

void Inform(wxWindow* parent, const wxString& title, const wxString& message, bool warning) {
    wxMessageDialog dialog(parent, message, title, wxOK | (warning ? wxICON_WARNING : wxICON_INFORMATION));
    dialog.SetOKLabel(L(wxT("好"), wxT("OK")));
    ShowModalDialog(dialog);
}

bool CanClosePage(wxCloseEvent& event) {
    if (g_openDialogs > 0 && event.CanVeto()) {
        event.Veto();
        wxBell();
        return false;
    }
    return true;
}

void InstallFullScreenKeys(wxFrame* frame) {
    frame->Bind(wxEVT_CHAR_HOOK, [frame](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_F11)
            frame->ShowFullScreen(!frame->IsFullScreen());
        else if (event.GetKeyCode() == WXK_ESCAPE && frame->IsFullScreen())
            frame->ShowFullScreen(false);
        else
            event.Skip();
    });
}

Widgets::FlatButton* MakeFullScreenButton(wxFrame* frame, wxWindow* parent) {
    Widgets::FlatButton* button = MakeHeaderButton(parent);
    button->SetToolTip(L(wxT("快捷鍵 F11；全螢幕時按 Esc 離開"), wxT("Shortcut F11; Esc leaves full screen")));
    button->Bind(wxEVT_BUTTON, [frame](wxCommandEvent&) { frame->ShowFullScreen(!frame->IsFullScreen()); });
    auto sync = [frame, button] {
        button->SetLabel(frame->IsFullScreen() ? L(wxT("  結束全螢幕  "), wxT("  Exit full screen  ")) : L(wxT("  全螢幕  "), wxT("  Full screen  ")));
        button->GetParent()->Layout();
    };
    frame->Bind(wxEVT_SIZE, [sync](wxSizeEvent& event) {
        sync();
        event.Skip();
    });
    sync();
    return button;
}

void ShowLike(wxFrame* next, const wxFrame* current, std::function<void()> onShown) {
    const bool fullScreen = current->IsFullScreen();
    const bool maximized = current->IsMaximized();
    Widgets::FadeIn(next, [next, fullScreen, maximized] {
        if (fullScreen) {
            if (next->IsFullScreen()) next->Show();
            else next->ShowFullScreen(true);
        } else {
            if (next->IsFullScreen()) next->ShowFullScreen(false);
            if (maximized) next->Maximize();
            else if (next->IsMaximized()) next->Restore();
            next->Show();
        }
    }, std::move(onShown));
}

wxString FormatPrice(int amount) {
    wxString digits = wxString::Format(wxT("%d"), amount < 0 ? -amount : amount);
    wxString grouped;
    int count = 0;
    for (int i = (int)digits.length() - 1; i >= 0; --i) {
        grouped.Prepend(digits[i]);
        if (++count % 3 == 0 && i > 0) grouped.Prepend(wxT(','));
    }
    return (amount < 0 ? wxT("-NT$") : wxT("NT$")) + grouped;
}

Widgets::FlatButton* MakePrimaryButton(wxWindow* parent, const wxString& label, int pointSize) {
    return new Widgets::FlatButton(parent, label, Widgets::FlatButton::Style::Primary, pointSize, 46);
}

Widgets::FlatButton* MakeSecondaryButton(wxWindow* parent, const wxString& label, int pointSize) {
    return new Widgets::FlatButton(parent, label, Widgets::FlatButton::Style::Secondary, pointSize, 40);
}

Widgets::FlatButton* MakeHeaderButton(wxWindow* parent) {
    return new Widgets::FlatButton(parent, wxEmptyString, Widgets::FlatButton::Style::OnDark, 11, 40);
}

wxStaticText* MakeLabel(wxWindow* parent, const wxString& text, int pointSize,
                        bool bold, const wxColour& colour) {
    wxStaticText* label = new wxStaticText(parent, wxID_ANY, text);
    label->SetFont(Font(pointSize, bold));
    label->SetForegroundColour(colour);
    return label;
}

wxWindow* MakeEyebrow(wxWindow* parent, const wxString& text) {
    wxString spaced;
    for (size_t i = 0; i < text.length(); ++i) spaced << text[i] << (i + 1 < text.length() ? wxT("\u2009") : wxT(""));
    wxPanel* pill = new wxPanel(parent, wxID_ANY);
    pill->SetBackgroundStyle(wxBG_STYLE_PAINT);
    const wxFont font = Font(8, true);
    wxClientDC measure(pill);
    measure.SetFont(font);
    const wxSize t = measure.GetTextExtent(spaced);
    pill->SetMinSize(wxSize(t.x + pill->FromDIP(22), t.y + pill->FromDIP(8)));
    pill->Bind(wxEVT_PAINT, [pill, spaced, font](wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(pill);
        dc.SetBackground(wxBrush(pill->GetParent()->GetBackgroundColour()));
        dc.Clear();
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;
        const wxSize s = pill->GetClientSize();
        gc->SetPen(wxPen(wxColour(242, 106, 33, 90), 1));
        gc->SetBrush(wxBrush(wxColour(255, 241, 232)));
        gc->DrawRoundedRectangle(0.5, 0.5, s.x - 1, s.y - 1, (s.y - 1) / 2.0);
        wxDouble tw, th;
        gc->SetFont(font, kOrangeDark);
        gc->GetTextExtent(spaced, &tw, &th);
        gc->DrawText(spaced, (s.x - tw) / 2, (s.y - th) / 2);
    });
    return pill;
}

Widgets::Card* MakeCard(wxWindow* parent, bool hoverable) {
    return new Widgets::Card(parent, hoverable);
}

void FitFrameToContent(wxTopLevelWindow* window, wxWindow* content, const wxSize& preferredDip) {
    wxSize needed = content->GetBestSize();
    wxSize preferred = window->FromDIP(preferredDip);
    window->SetMinClientSize(needed);
    window->SetClientSize(wxSize(std::max(needed.x, preferred.x), std::max(needed.y, preferred.y)));
}

wxPanel* MakeHeader(wxWindow* parent, const wxString& title, const wxString& subtitle,
                    wxBoxSizer** rightSizer) {
    wxPanel* header = new wxPanel(parent, wxID_ANY);
    header->SetBackgroundColour(kNavy);

    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);

    wxPanel* accent = new wxPanel(header, wxID_ANY, wxDefaultPosition, header->FromDIP(wxSize(5, 40)));
    accent->SetBackgroundColour(kOrange);
    row->Add(accent, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, header->FromDIP(24));

    wxBoxSizer* texts = new wxBoxSizer(wxVERTICAL);
    texts->Add(MakeLabel(header, title, 17, true, kOnDark));
    texts->Add(MakeLabel(header, subtitle, 10, false, kOnDarkMuted), 0, wxTOP, header->FromDIP(2));
    row->Add(texts, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, header->FromDIP(14));

    wxBoxSizer* right = new wxBoxSizer(wxHORIZONTAL);
    row->Add(right, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, header->FromDIP(24));
    if (rightSizer) *rightSizer = right;

    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(row, 1, wxEXPAND | wxTOP | wxBOTTOM, header->FromDIP(16));
    header->SetSizer(outer);
    return header;
}

}
