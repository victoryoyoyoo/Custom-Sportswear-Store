#pragma once
#include <wx/wx.h>
#include <functional>
#include <vector>

// Custom-drawn controls and the small animation helper they share. They paint
// themselves with wxGraphicsContext (anti-aliased curves, alpha) because the
// native Win32 controls can't be restyled or animated.
namespace Widgets {

// Runs a short animation: calls onStep(t) ~60 times a second with t eased from
// 0 to 1, then onDone(). Starting it again restarts it.
class Tween : public wxTimer {
public:
    void Start(int durationMs, std::function<void(double)> onStep, std::function<void()> onDone = {});
    void Notify() override;

private:
    wxLongLong m_startMs;
    int m_durationMs = 0;
    std::function<void(double)> m_onStep;
    std::function<void()> m_onDone;
};

double EaseOut(double t);                                          // fast start, long soft landing
wxColour Mix(const wxColour& from, const wxColour& to, double t);  // 0 -> from, 1 -> to

// Button with smooth hover/press colour changes. Primary buttons are pills and
// can carry an arrow in its own little circle that nudges forward on hover.
// Sends a normal wxEVT_BUTTON, so it's bound exactly like a wxButton.
class FlatButton : public wxControl {
public:
    enum class Style { Primary, Secondary, OnDark };

    FlatButton(wxWindow* parent, const wxString& label, Style style, int pointSize, int heightDip);

    void SetLabel(const wxString& label) override;
    bool Enable(bool enable = true) override;
    void ShowArrow(bool show = true);
    void Flash(const wxColour& colour);  // brief glow, e.g. when something lands in the cart

protected:
    wxSize DoGetBestClientSize() const override;

private:
    void OnPaint(wxPaintEvent& event);
    void AnimateHoverTo(double target);
    void Click();

    Style m_style;
    wxColour m_base, m_hoverColour, m_text;
    double m_hover = 0.0, m_flash = 0.0;
    wxColour m_flashColour;
    bool m_pressed = false, m_arrow = false;
    int m_heightDip;
    Tween m_hoverTween, m_flashTween;
};

// Content card: a soft ambient shadow, a thin tinted shell and a white core
// with concentric rounded corners. Hoverable cards lift (deeper shadow, warm
// outline) while the pointer is anywhere over them, children included.
class Card : public wxPanel {
public:
    explicit Card(wxWindow* parent, bool hoverable = false);
    static int InsetDip() { return 8; }  // shadow + shell; keep content further in than this

private:
    void OnPaint(wxPaintEvent& event);
    void TrackHover();

    double m_hover = 0.0;
    bool m_hovered = false;
    Tween m_tween;
    wxTimer m_leaveCheck;  // leave events also fire when moving onto a child
};

// A row of pill-shaped choices (sizes). One is always selected.
class ChipPicker : public wxPanel {
public:
    ChipPicker(wxWindow* parent, const std::vector<wxString>& labels, int selection);
    int GetSelection() const { return m_selected; }
    void OnSelectionChanged(std::function<void(int)> callback) { m_onChange = std::move(callback); }

private:
    wxRect ChipRect(int index) const;
    int HitTest(const wxPoint& p) const;
    void Select(int index);
    void OnPaint(wxPaintEvent& event);

    std::vector<wxString> m_labels;
    int m_selected, m_hovered = -1, m_chipWidth = 0;
    std::function<void(int)> m_onChange;
};

// Rounded progress bar whose fill glides to the new value.
class ProgressBar : public wxPanel {
public:
    explicit ProgressBar(wxWindow* parent);
    void SetValue(double value);  // 0..1

private:
    void OnPaint(wxPaintEvent& event);
    double m_shown = 0.0, m_target = 0.0;
    Tween m_tween;
};

// "① 購物車 — ② 填寫資料 — ③ 完成" with the current step highlighted.
class StepIndicator : public wxPanel {
public:
    StepIndicator(wxWindow* parent, const std::vector<wxString>& steps, int current);

private:
    void OnPaint(wxPaintEvent& event);
    std::vector<wxString> m_steps;
    int m_current;
};

// Small notification that slides up in the corner of `owner`, stays a moment
// and fades away. Clicking it runs onClick.
void ShowToast(wxFrame* owner, const wxString& title, const wxString& detail, std::function<void()> onClick = {});

// Shows `window` fading in from transparent; `show` does the actual
// Show()/ShowFullScreen(), `onShown` runs once it is fully opaque.
void FadeIn(wxTopLevelWindow* window, std::function<void()> show, std::function<void()> onShown = {});

}  // namespace Widgets
