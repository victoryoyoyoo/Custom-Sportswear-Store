#pragma once
#include <wx/wx.h>
#include <functional>
#include "Catalog.h"
#include "Widgets.h"

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
    int m_previous = -1;
    double m_ring = 1.0;
    Widgets::Tween m_ringTween;
    std::function<void(int)> m_onChange;
};
