#pragma once
#include <wx/wx.h>
#include <functional>
#include <vector>

namespace Widgets {

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

double EaseOut(double t);
wxColour Mix(const wxColour& from, const wxColour& to, double t);

class FlatButton : public wxControl {
public:
    enum class Style { Primary, Secondary, OnDark };

    FlatButton(wxWindow* parent, const wxString& label, Style style, int pointSize, int heightDip);

    void SetLabel(const wxString& label) override;
    bool Enable(bool enable = true) override;
    void ShowArrow(bool show = true);
    void Flash(const wxColour& colour);

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

class Card : public wxPanel {
public:
    explicit Card(wxWindow* parent, bool hoverable = false);
    static int InsetDip() { return 8; }

private:
    void OnPaint(wxPaintEvent& event);
    void TrackHover();

    double m_hover = 0.0;
    bool m_hovered = false;
    Tween m_tween;
    wxTimer m_leaveCheck;
};

class ChipPicker : public wxPanel {
public:
    ChipPicker(wxWindow* parent, const std::vector<wxString>& labels, int selection);
    int GetSelection() const { return m_selected; }
    void SetSelection(int index) { Select(index); }
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

class HeartToggle : public wxControl {
public:
    HeartToggle(wxWindow* parent, bool on, int sizeDip = 34);
    bool IsOn() const { return m_on; }
    void SetOn(bool on);
    void OnToggled(std::function<void(bool)> callback) { m_onToggle = std::move(callback); }

protected:
    wxSize DoGetBestClientSize() const override { return FromDIP(wxSize(m_sizeDip, m_sizeDip)); }

private:
    void OnPaint(wxPaintEvent& event);
    bool m_on;
    int m_sizeDip;
    double m_pop = 0.0, m_hover = 0.0;
    std::function<void(bool)> m_onToggle;
    Tween m_popTween, m_hoverTween;
};

class ProgressBar : public wxPanel {
public:
    explicit ProgressBar(wxWindow* parent);
    void SetValue(double value);

private:
    void OnPaint(wxPaintEvent& event);
    double m_shown = 0.0, m_target = 0.0;
    Tween m_tween;
};

class StepIndicator : public wxPanel {
public:
    StepIndicator(wxWindow* parent, const std::vector<wxString>& steps, int current);

private:
    void OnPaint(wxPaintEvent& event);
    std::vector<wxString> m_steps;
    int m_current;
};

void ShowToast(wxFrame* owner, const wxString& title, const wxString& detail, std::function<void()> onClick = {});

void FadeIn(wxTopLevelWindow* window, std::function<void()> show, std::function<void()> onShown = {});

}
