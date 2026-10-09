#pragma once
#include <wx/wx.h>
#include <functional>
#include "Widgets.h"

namespace Theme {
    const wxColour kNavy(14, 23, 42);
    const wxColour kNavyLight(32, 46, 76);
    const wxColour kOrange(242, 106, 33);
    const wxColour kOrangeDark(214, 86, 18);
    const wxColour kPage(243, 245, 249);
    const wxColour kCard(255, 255, 255);
    const wxColour kBorder(222, 227, 235);
    const wxColour kText(28, 36, 52);
    const wxColour kMuted(110, 120, 138);
    const wxColour kOnDark(255, 255, 255);
    const wxColour kOnDarkMuted(168, 181, 204);
    const wxColour kSuccess(22, 140, 84);
    const wxColour kError(200, 40, 40);

    wxFont Font(int pointSize, bool bold = false);

    wxString AssetPath(const wxString& fileName);

    class ImagePanel : public wxPanel {
    public:
        using Renderer = std::function<wxBitmap(const wxSize& pixels)>;
        ImagePanel(wxWindow* parent, const wxSize& minDipSize, Renderer renderer);

        void Rerender();

        void PlayIntro(int delayMs);

    private:
        void OnPaint(wxPaintEvent& event);
        void StartCrossfade(const wxImage& from, const wxImage& to, int durationMs);

        Renderer m_renderer;
        wxBitmap m_bitmap;
        wxImage m_fadeFrom, m_fadeTo;
        bool m_pending = false;
        int m_introDelay = -1;
        Widgets::Tween m_fade;
        wxTimer m_introTimer;
        wxImage m_introFrom, m_introTo;
    };

    void LimitWidth(wxWindow* host, wxSizerItem* item, int maxDip, int minMarginDip);

    int ShowModalDialog(wxDialog& dialog);

    bool Confirm(wxWindow* parent, const wxString& title, const wxString& message, const wxString& yes);
    void Inform(wxWindow* parent, const wxString& title, const wxString& message, bool warning = false);
    bool CanClosePage(wxCloseEvent& event);

    void InstallFullScreenKeys(wxFrame* frame);

    Widgets::FlatButton* MakeFullScreenButton(wxFrame* frame, wxWindow* parent);

    void ShowLike(wxFrame* next, const wxFrame* current, std::function<void()> onShown = {});

    wxString FormatPrice(int amount);

    Widgets::FlatButton* MakePrimaryButton(wxWindow* parent, const wxString& label, int pointSize = 12);
    Widgets::FlatButton* MakeSecondaryButton(wxWindow* parent, const wxString& label, int pointSize = 11);
    wxStaticText* MakeLabel(wxWindow* parent, const wxString& text, int pointSize,
                            bool bold = false, const wxColour& colour = kText);

    wxWindow* MakeEyebrow(wxWindow* parent, const wxString& text);

    Widgets::Card* MakeCard(wxWindow* parent, bool hoverable = false);

    void FitFrameToContent(wxTopLevelWindow* window, wxWindow* content, const wxSize& preferredDip);

    Widgets::FlatButton* MakeHeaderButton(wxWindow* parent);

    wxPanel* MakeHeader(wxWindow* parent, const wxString& title, const wxString& subtitle,
                        wxBoxSizer** rightSizer = nullptr);
}
