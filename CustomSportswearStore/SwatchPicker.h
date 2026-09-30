#pragma once
#include <wx/wx.h>
#include <functional>
#include "Catalog.h"
#include "Widgets.h"

// A custom-drawn row of colour dots, one per colourway. wxWidgets has no
// built-in "colour swatch picker", so this control paints the dots itself
// (wxEVT_PAINT) and turns mouse clicks into a selection callback.
class SwatchPicker : public wxPanel {
public:
    SwatchPicker(wxWindow* parent, int columns);

    int GetSelection() const { return m_selected; }
    void SetSelection(int index);
    void OnSelectionChanged(std::function<void(int)> callback) { m_onChange = std::move(callback); }

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    int HitTest(const wxPoint& pos) const;
    wxRect CellRect(int index) const;

    int m_columns;
    int m_selected = 0;
    int m_hovered = -1;
    int m_previous = -1;      // selection the ring is moving away from
    double m_ring = 1.0;      // 0..1 progress of the selection-ring animation
    Widgets::Tween m_ringTween;
    std::function<void(int)> m_onChange;
};
