#pragma once
#include <wx/wx.h>
#include <functional>
#include <memory>
#include "Showcase.h"
#include "Widgets.h"

class Turntable : public wxWindow {
public:
    using Builder = std::function<std::unique_ptr<Showcase::Model>(const wxSize& pixels)>;

    Turntable(wxWindow* parent, const wxSize& minDipSize, Builder builder);

    void Rebuild(bool crossfade);

    void TurnTo(double angle);
    void Spin(int delayMs);

    void OnSideChanged(std::function<void(int side)> handler) { m_onSide = std::move(handler); }

private:
    void OnPaint(wxPaintEvent& event);
    void SetAngle(double angle, bool moving);
    void Settle();
    void EnsureModel();
    void StartInertia();

    Builder m_builder;
    std::unique_ptr<Showcase::Model> m_model;
    wxSize m_modelSize;
    bool m_modelStale = true;

    double m_angle = 0;
    bool m_moving = false;
    Showcase::Frame m_frame;
    bool m_frameStale = true;
    int m_side = 0;
    std::function<void(int)> m_onSide;

    bool m_dragging = false;
    int m_lastX = 0;
    wxLongLong m_lastMoveMs;
    double m_velocity = 0;
    wxTimer m_coast;
    wxLongLong m_coastMs;

    Widgets::Tween m_turn, m_fade;
    wxTimer m_spinDelay;
    wxBitmap m_fadeFrom;
    double m_fadeLeft = 0;
    double m_hint = 1;
    Widgets::Tween m_hintTween;
    bool m_rebuildPending = false;
    bool m_rebuildFade = false;
};
