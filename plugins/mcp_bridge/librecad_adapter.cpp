#include "librecad_adapter.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

void LibreCadDrawingAdapter::drawArc(const Arc& arc) {
    QPointF c = arc.center;
    m_dpi->addArc(&c, arc.radius, arc.startAngle, arc.endAngle);
}

void LibreCadDrawingAdapter::drawEllipse(const Ellipse& ellipse) {
    QPointF c = ellipse.center;
    QPointF e = ellipse.majorAxisEnd;
    m_dpi->addEllipse(&c, &e, ellipse.ratio, ellipse.startAngle, ellipse.endAngle);
}

void LibreCadDrawingAdapter::drawPolyline(const Polyline& polyline) {
    std::vector<Plug_VertexData> pts;
    for (const auto& p : polyline.points) {
        pts.push_back(Plug_VertexData(p, 0.0));
    }
    m_dpi->addPolyline(pts, polyline.closed);
}

void LibreCadDrawingAdapter::addText(const Text& text) {
    QPointF p = text.position;
    m_dpi->addText(text.content, "standard", &p, text.size, 0.0, DPI::HAlignLeft, DPI::VAlignBottom);
}

void LibreCadDrawingAdapter::setLayer(const QString& name) {
    m_dpi->setLayer(name);
}

void LibreCadDrawingAdapter::deleteLayer(const QString& name) {
    m_dpi->deleteLayer(name);
}

// --- Interior Architecture ---

void LibreCadDrawingAdapter::drawWall(const Wall& wall) {
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
        m_dpi->addLine(const_cast<QPointF*>(&opening.start), const_cast<QPointF*>(&opening.end));
        double radius = std::sqrt(std::pow(opening.end.x()-opening.start.x(), 2) + std::pow(opening.end.y()-opening.start.y(), 2));
        double angle = std::atan2(opening.end.y()-opening.start.y(), opening.end.x()-opening.start.x()) * 180.0 / M_PI;
        m_dpi->addArc(const_cast<QPointF*>(&opening.start), radius, angle, angle + 90.0);
    } else {
        m_dpi->addLine(const_cast<QPointF*>(&opening.start), const_cast<QPointF*>(&opening.end));
    }
}

// --- Military Tools ---

void LibreCadDrawingAdapter::drawTacticalSymbol(const TacticalSymbol& sym) {
    m_dpi->setLayer(sym.identity == "hostile" ? "MIL_HOSTILE" : "MIL_FRIENDLY");
    
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
    
    m_dpi->addText(sym.unitType, "standard", const_cast<QPointF*>(&sym.position), s * 0.5, 0.0, DPI::HAlignCenter, DPI::VAlignMiddle);
}

void LibreCadDrawingAdapter::drawTacticalLine(const TacticalLine& line) {
    m_dpi->setLayer("MIL_TACTICAL");
    std::vector<Plug_VertexData> pts;
    for (const auto& p : line.points) pts.push_back(Plug_VertexData(p, 0.0));
    m_dpi->addPolyline(pts, false);
}

// --- Queries ---

QStringList LibreCadDrawingAdapter::getLayers() { return m_dpi->getAllLayer(); }
QStringList LibreCadDrawingAdapter::getBlocks() { return m_dpi->getAllBlocks(); }
QString LibreCadDrawingAdapter::getCurrentLayer() { return m_dpi->getCurrentLayer(); }

void LibreCadDrawingAdapter::commit() {
    m_dpi->updateView();
}

} // namespace mcp
