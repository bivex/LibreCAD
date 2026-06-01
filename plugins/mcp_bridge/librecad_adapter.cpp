#include "librecad_adapter.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_math.h"

namespace mcp {

LibreCadDrawingAdapter::LibreCadDrawingAdapter(Document_Interface* dpi, QWidget* parent)
    : m_dpi(dpi), m_parent(parent) {}

void LibreCadDrawingAdapter::drawLine(const Line& line) {
    QPointF s = line.start; QPointF e = line.end;
    m_dpi->addLine(&s, &e);
}

void LibreCadDrawingAdapter::drawCircle(const Circle& circle) {
    QPointF c = circle.center;
    m_dpi->addCircle(&c, circle.radius);
}

void LibreCadDrawingAdapter::addText(const Text& text) {
    QPointF p = text.position;
    m_dpi->addText(text.content, "standard", &p, text.size, 0.0, DPI::HLeft, DPI::VBase);
}

void LibreCadDrawingAdapter::setLayer(const QString& name) {
    m_dpi->setLayer(name);
}

// --- Interior Implementation ---

void LibreCadDrawingAdapter::drawWall(const Wall& wall) {
    // A wall is drawn as two parallel lines based on thickness
    double dx = wall.end.x() - wall.start.x();
    double dy = wall.end.y() - wall.start.y();
    double length = std::sqrt(dx*dx + dy*dy);
    if (length < 1e-6) return;

    double ux = -dy / length * (wall.thickness / 2.0);
    double uy = dx / length * (wall.thickness / 2.0);

    QPointF p1(wall.start.x() + ux, wall.start.y() + uy);
    QPointF p2(wall.end.x() + ux, wall.end.y() + uy);
    QPointF p3(wall.start.x() - ux, wall.start.y() - uy);
    QPointF p4(wall.end.x() - ux, wall.end.y() - uy);

    m_dpi->addLine(&p1, &p2);
    m_dpi->addLine(&p3, &p4);
}

void LibreCadDrawingAdapter::drawOpening(const Opening& opening) {
    if (opening.type == "door") {
        // Line for the door and an arc for the swing
        m_dpi->addLine(const_cast<QPointF*>(&opening.start), const_cast<QPointF*>(&opening.end));
        double radius = std::sqrt(std::pow(opening.end.x()-opening.start.x(), 2) + std::pow(opening.end.y()-opening.start.y(), 2));
        double angle = std::atan2(opening.end.y()-opening.start.y(), opening.end.x()-opening.start.x()) * 180.0 / M_PI;
        m_dpi->addArc(const_cast<QPointF*>(&opening.start), radius, angle, angle + 90.0);
    } else {
        // Window is two lines across the opening
        m_dpi->addLine(const_cast<QPointF*>(&opening.start), const_cast<QPointF*>(&opening.end));
        // Add a second offset line for window detail
    }
}

// --- Military Implementation ---

void LibreCadDrawingAdapter::drawTacticalSymbol(const TacticalSymbol& sym) {
    m_dpi->setLayer(sym.identity == "hostile" ? "MIL_HOSTILE" : "MIL_FRIENDLY");
    
    // Draw box for friendly, diamond for hostile (simplified)
    double s = sym.size / 2.0;
    if (sym.identity == "hostile") {
        QPointF p1(sym.position.x(), sym.position.y() + s);
        QPointF p2(sym.position.x() + s, sym.position.y());
        QPointF p3(sym.position.x(), sym.position.y() - s);
        QPointF p4(sym.position.x() - s, sym.position.y());
        m_dpi->addLine(&p1, &p2); m_dpi->addLine(&p2, &p3); m_dpi->addLine(&p3, &p4); m_dpi->addLine(&p4, &p1);
    } else {
        QPointF p1(sym.position.x() - s, sym.position.y() - s);
        QPointF p2(sym.position.x() + s, sym.position.y() - s);
        QPointF p3(sym.position.x() + s, sym.position.y() + s);
        QPointF p4(sym.position.x() - s, sym.position.y() + s);
        m_dpi->addLine(&p1, &p2); m_dpi->addLine(&p2, &p3); m_dpi->addLine(&p3, &p4); m_dpi->addLine(&p4, &p1);
    }
    
    // Add unit type text
    m_dpi->addText(sym.unitType, "standard", const_cast<QPointF*>(&sym.position), s * 0.5, 0.0, DPI::HCenter, DPI::VMiddle);
}

void LibreCadDrawingAdapter::drawTacticalLine(const TacticalLine& line) {
    m_dpi->setLayer("MIL_TACTICAL");
    std::vector<Plug_VertexData> pts;
    for (const auto& p : line.points) pts.push_back(Plug_VertexData(p, 0.0));
    m_dpi->addPolyline(pts, false);
    
    if (line.type == "axis_of_advance") {
        // Add arrow head logic here
    }
}

QStringList LibreCadDrawingAdapter::getLayers() { return m_dpi->getAllLayer(); }

void LibreCadDrawingAdapter::commit() {
    QC_ApplicationWindow* app = QC_ApplicationWindow::getAppWindow().get();
    if (QC_MDIWindow* w = app->getCurrentMDIWindow()) w->getGraphicView()->redraw();
}

} // namespace mcp
