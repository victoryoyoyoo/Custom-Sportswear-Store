#include "Theme.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <memory>
#include <algorithm>

namespace Theme {

wxFont Font(int pointSize, bool bold) {
    wxFontInfo info(pointSize);
    info.FaceName(wxT("Microsoft JhengHei UI"));
    if (bold) info.Bold();
    return wxFont(info);
}

wxString AssetPath(const wxString& fileName) {
    // Candidate roots: current directory, then the exe folder and up to three
    // parents (x64\Debug\ -> project -> solution).
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

wxBitmap LoadFittedPixels(const wxString& fileName, const wxSize& maxPixels) {
    const int maxW = std::max(1, maxPixels.GetWidth());
    const int maxH = std::max(1, maxPixels.GetHeight());

    wxImage img(AssetPath(fileName), wxBITMAP_TYPE_PNG);
    if (!img.IsOk()) {
        // Missing asset: show a neutral placeholder instead of crashing.
        wxImage blank(maxW, maxH);
        blank.SetRGB(wxRect(0, 0, maxW, maxH), kBorder.Red(), kBorder.Green(), kBorder.Blue());
        return wxBitmap(blank);
    }
    const double ratio = std::min((double)maxW / img.GetWidth(), (double)maxH / img.GetHeight());
    const int w = std::max(1, (int)(img.GetWidth() * ratio));
    const int h = std::max(1, (int)(img.GetHeight() * ratio));

    // Resample through the graphics backend rather than wxImage::Rescale,
    // which leaves contour lines across smooth gradients (soft shadows, glows).
    wxBitmap result(w, h, 24);
    {
        wxMemoryDC dc(result);
        dc.SetBackground(wxBrush(kCard));
        dc.Clear();
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (gc) {
            gc->SetInterpolationQuality(wxINTERPOLATION_BEST);
            // Overdraw by 1px on every side: the resampler blends edge pixels
            // with the background, which otherwise shows as a thin light border.
            gc->DrawBitmap(wxBitmap(img), -1, -1, w + 2, h + 2);
        }
    }
    return result;
}

ImagePanel::ImagePanel(wxWindow* parent, const wxSize& minDipSize, Renderer renderer)
    : wxPanel(parent, wxID_ANY), m_renderer(std::move(renderer)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(FromDIP(minDipSize));
    Bind(wxEVT_PAINT, &ImagePanel::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        Rerender();
        event.Skip();
    });
}

ImagePanel* ImagePanel::ForAsset(wxWindow* parent, const wxString& fileName, const wxSize& minDipSize) {
    return new ImagePanel(parent, minDipSize, [fileName](const wxSize& px) { return LoadFittedPixels(fileName, px); });
}

void ImagePanel::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();
    if (m_bitmap.IsOk()) {
        const wxSize area = GetClientSize();
        dc.DrawBitmap(m_bitmap, (area.x - m_bitmap.GetWidth()) / 2, (area.y - m_bitmap.GetHeight()) / 2);
    }
}

void ImagePanel::Rerender(bool crossfade) {
    // A resize or a burst of keystrokes can ask many times in a row; render
    // once, after the current events have been handled.
    m_pendingFade = m_pendingFade || crossfade;
    if (m_pending) return;
    m_pending = true;
    CallAfter([this] {
        m_pending = false;
        const bool fade = m_pendingFade;
        m_pendingFade = false;
        const wxSize area = GetClientSize();
        if (area.x < 8 || area.y < 8 || !m_renderer) return;
        wxBitmap next = m_renderer(area);

        if (!fade || !m_bitmap.IsOk() || m_bitmap.GetSize() != next.GetSize()) {
            m_fade.Stop();
            m_bitmap = next;
            Refresh();
            return;
        }
        // Dissolve: blend the two frames pixel by pixel on every tick.
        m_fadeFrom = m_bitmap.ConvertToImage();
        m_fadeTo = next.ConvertToImage();
        m_fade.Start(200, [this](double t) {
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
    });
}

void LimitWidth(wxWindow* host, wxSizerItem* item, int maxDip, int minMarginDip) {
    host->Bind(wxEVT_SIZE, [host, item, maxDip, minMarginDip](wxSizeEvent& event) {
        const int width = host->GetClientSize().x;
        const int margin = std::max(host->FromDIP(minMarginDip), (width - host->FromDIP(maxDip)) / 2);
        if (item->GetBorder() != margin) item->SetBorder(margin);
        event.Skip();  // the default handler then lays the sizer out
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
    button->SetToolTip(wxT("快捷鍵 F11；全螢幕時按 Esc 離開"));
    button->Bind(wxEVT_BUTTON, [frame](wxCommandEvent&) { frame->ShowFullScreen(!frame->IsFullScreen()); });
    // The state can change from the keyboard or from ShowLike(), so the label
    // follows the window size instead of the click.
    auto sync = [frame, button] {
        button->SetLabel(frame->IsFullScreen() ? wxT("  結束全螢幕  ") : wxT("  全螢幕  "));
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
            // ShowFullScreen(true) does nothing on a frame that is already
            // full screen but hidden, so show it explicitly in that case.
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
    // Spaced-out capitals in a pale pill, drawn by hand for the letter spacing.
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
    // Never smaller than what the sizers need (so nothing gets squashed at
    // 125%/150% scaling or with larger fonts), otherwise the preferred size.
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

    // Orange accent bar, same motif as the welcome banner.
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

}  // namespace Theme
