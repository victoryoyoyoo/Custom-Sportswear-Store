#pragma once
#include <wx/wx.h>
#include <functional>
#include "Widgets.h"

// Shared look & feel: one palette, one font family, and a few helpers so every
// window in the app is built from the same pieces instead of ad-hoc colours.
namespace Theme {
    // Palette
    const wxColour kNavy(14, 23, 42);           // header bars, primary text on light bg
    const wxColour kNavyLight(32, 46, 76);
    const wxColour kOrange(242, 106, 33);       // brand accent / primary buttons
    const wxColour kOrangeDark(214, 86, 18);
    const wxColour kPage(243, 245, 249);        // window background
    const wxColour kCard(255, 255, 255);        // content cards
    const wxColour kBorder(222, 227, 235);
    const wxColour kText(28, 36, 52);
    const wxColour kMuted(110, 120, 138);
    const wxColour kOnDark(255, 255, 255);
    const wxColour kOnDarkMuted(168, 181, 204);
    const wxColour kSuccess(22, 140, 84);
    const wxColour kError(200, 40, 40);

    wxFont Font(int pointSize, bool bold = false);

    // Finds a file inside the "assets" folder whether the app is started from
    // Visual Studio (working dir = solution dir) or by double-clicking the exe.
    wxString AssetPath(const wxString& fileName);

    // Shows a picture that grows with its panel. Whenever the panel changes
    // size the renderer is asked for a new bitmap at exactly that many
    // physical pixels, so the picture stays sharp from a small window up to
    // full screen.
    class ImagePanel : public wxPanel {
    public:
        using Renderer = std::function<wxBitmap(const wxSize& pixels)>;
        ImagePanel(wxWindow* parent, const wxSize& minDipSize, Renderer renderer);

        // Call when what the renderer draws has changed.
        void Rerender();

        // Next time a picture is drawn, start from the background colour and
        // fade it in after `delayMs` (staggered entrance on the product list).
        void PlayIntro(int delayMs);

    private:
        void OnPaint(wxPaintEvent& event);
        void StartCrossfade(const wxImage& from, const wxImage& to, int durationMs);

        Renderer m_renderer;
        wxBitmap m_bitmap;
        wxImage m_fadeFrom, m_fadeTo;   // crossfade endpoints
        bool m_pending = false;
        int m_introDelay = -1;          // >= 0 while an intro is waiting
        Widgets::Tween m_fade;
        wxTimer m_introTimer;
        wxImage m_introFrom, m_introTo; // what the intro fades between
    };

    // Keeps a sizer item (added with wxLEFT | wxRIGHT) no wider than maxDip by
    // growing its side borders, so on a big or full screen the content stays a
    // readable width and sits in the middle instead of stretching edge to edge.
    void LimitWidth(wxWindow* host, wxSizerItem* item, int maxDip, int minMarginDip);

    // Dialogs are stack objects owned by a page. If the page were closed from
    // the taskbar while ShowModal() is running, wx would delete the dialog with
    // it. ShowModalDialog counts open dialogs and CanClosePage vetoes the close
    // until they are gone.
    int ShowModalDialog(wxDialog& dialog);

    // Question / notice boxes with Chinese button labels (the system ones
    // would say Yes / No / OK). Confirm returns true for `yes`.
    bool Confirm(wxWindow* parent, const wxString& title, const wxString& message, const wxString& yes);
    void Inform(wxWindow* parent, const wxString& title, const wxString& message, bool warning = false);
    bool CanClosePage(wxCloseEvent& event);  // vetoes and returns false while a dialog is open

    // F11 toggles full screen, Esc leaves it.
    void InstallFullScreenKeys(wxFrame* frame);

    // Header button that toggles full screen and shows the current state.
    Widgets::FlatButton* MakeFullScreenButton(wxFrame* frame, wxWindow* parent);

    // Page hand-over: fades `next` in using the same window state as `current`
    // (full screen / maximised / normal), then runs onShown, typically hiding
    // or closing `current` so the desktop never flashes between pages.
    void ShowLike(wxFrame* next, const wxFrame* current, std::function<void()> onShown = {});

    // 1280 -> "NT$1,280"
    wxString FormatPrice(int amount);

    Widgets::FlatButton* MakePrimaryButton(wxWindow* parent, const wxString& label, int pointSize = 12);
    Widgets::FlatButton* MakeSecondaryButton(wxWindow* parent, const wxString& label, int pointSize = 11);
    wxStaticText* MakeLabel(wxWindow* parent, const wxString& text, int pointSize,
                            bool bold = false, const wxColour& colour = kText);

    // Tiny letter-spaced tag that sits above a heading ("SHOP ALL").
    wxWindow* MakeEyebrow(wxWindow* parent, const wxString& text);

    // White rounded content card; hoverable cards lift under the mouse.
    Widgets::Card* MakeCard(wxWindow* parent, bool hoverable = false);

    // Sizes a frame/dialog to at least its content's best size.
    void FitFrameToContent(wxTopLevelWindow* window, wxWindow* content, const wxSize& preferredDip);

    // Button styled for the dark header bar (used for the cart summary).
    Widgets::FlatButton* MakeHeaderButton(wxWindow* parent);

    // Dark header bar: title + subtitle on the left; `rightSizer` (may be
    // nullptr) receives a sizer the caller can put buttons into on the right.
    wxPanel* MakeHeader(wxWindow* parent, const wxString& title, const wxString& subtitle,
                        wxBoxSizer** rightSizer = nullptr);
}
