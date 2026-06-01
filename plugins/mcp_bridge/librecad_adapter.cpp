#include "librecad_adapter.h"
#include <cmath>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace mcp {

LibreCadDrawingAdapter::LibreCadDrawingAdapter(Document_Interface* dpi, QWidget* parent)
    : m_dpi(dpi), m_parent(parent) {
    qDebug() << "[MCP BRIDGE] LibreCadDrawingAdapter initialized.";
}

void LibreCadDrawingAdapter::drawLine(const Line& line) {
    qDebug() << "[MCP BRIDGE] drawLine called.";
    QPointF s = line.start; QPointF e = line.end;
    if (m_dpi) m_dpi->addLine(&s, &e);
}

void LibreCadDrawingAdapter::drawCircle(const Circle& circle) {
    qDebug() << "[MCP BRIDGE] drawCircle called.";
    QPointF c = circle.center;
    if (m_dpi) m_dpi->addCircle(&c, circle.radius);
}

void LibreCadDrawingAdapter::drawArc(const Arc& arc) {
    qDebug() << "[MCP BRIDGE] drawArc called.";
    QPointF c = arc.center;
    if (m_dpi) m_dpi->addArc(&c, arc.radius, arc.startAngle, arc.endAngle);
}

void LibreCadDrawingAdapter::drawEllipse(const Ellipse& ellipse) {
    qDebug() << "[MCP BRIDGE] drawEllipse called.";
    QPointF c = ellipse.center;
    QPointF e = ellipse.majorAxisEnd;
    if (m_dpi) m_dpi->addEllipse(&c, &e, ellipse.ratio, ellipse.startAngle, ellipse.endAngle);
}

void LibreCadDrawingAdapter::drawPolyline(const Polyline& polyline) {
    qDebug() << "[MCP BRIDGE] drawPolyline called.";
    std::vector<Plug_VertexData> pts;
    for (const auto& p : polyline.points) {
        pts.push_back(Plug_VertexData(p, 0.0));
    }
    if (m_dpi) m_dpi->addPolyline(pts, polyline.closed);
}

void LibreCadDrawingAdapter::addText(const Text& text) {
    qDebug() << "[MCP BRIDGE] addText called.";
    QPointF p = text.position;
    if (m_dpi) m_dpi->addText(text.content, "standard", &p, text.size, 0.0, DPI::HAlignCenter, DPI::VAlignMiddle);
}

void LibreCadDrawingAdapter::setLayer(const QString& name) {
    qDebug() << "[MCP BRIDGE] setLayer called:" << name;
    if (m_dpi) m_dpi->setLayer(name);
}

void LibreCadDrawingAdapter::deleteLayer(const QString& name) {
    qDebug() << "[MCP BRIDGE] deleteLayer called:" << name;
    if (m_dpi) m_dpi->deleteLayer(name);
}

// --- Interior Architecture ---

void LibreCadDrawingAdapter::drawWall(const Wall& wall) {
    qDebug() << "[MCP BRIDGE] drawWall called.";
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

    if (m_dpi) {
        m_dpi->addLine(&p1, &p2);
        m_dpi->addLine(&p3, &p4);
    }
}

void LibreCadDrawingAdapter::drawOpening(const Opening& opening) {
    qDebug() << "[MCP BRIDGE] drawOpening called.";
    if (m_dpi) {
        if (opening.type == "door") {
            m_dpi->addLine(const_cast<QPointF*>(&opening.start), const_cast<QPointF*>(&opening.end));
            double radius = std::sqrt(std::pow(opening.end.x()-opening.start.x(), 2) + std::pow(opening.end.y()-opening.start.y(), 2));
            double angle = std::atan2(opening.end.y()-opening.start.y(), opening.end.x()-opening.start.x()) * 180.0 / M_PI;
            m_dpi->addArc(const_cast<QPointF*>(&opening.start), radius, angle, angle + 90.0);
        } else {
            m_dpi->addLine(const_cast<QPointF*>(&opening.start), const_cast<QPointF*>(&opening.end));
        }
    }
}

// --- Military Tools ---

void LibreCadDrawingAdapter::drawTacticalSymbol(const TacticalSymbol& sym) {
    qDebug() << "[MCP BRIDGE] drawTacticalSymbol called.";
    if (!m_dpi) return;

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
    qDebug() << "[MCP BRIDGE] drawTacticalLine called.";
    if (!m_dpi) return;

    m_dpi->setLayer("MIL_TACTICAL");
    std::vector<Plug_VertexData> pts;
    for (const auto& p : line.points) pts.push_back(Plug_VertexData(p, 0.0));
    m_dpi->addPolyline(pts, false);
}

// --- Queries ---

QStringList LibreCadDrawingAdapter::getLayers() { return m_dpi ? m_dpi->getAllLayer() : QStringList(); }
QStringList LibreCadDrawingAdapter::getBlocks() { return m_dpi ? m_dpi->getAllBlocks() : QStringList(); }
QString LibreCadDrawingAdapter::getCurrentLayer() { return m_dpi ? m_dpi->getCurrentLayer() : QString(); }

void LibreCadDrawingAdapter::commit() {
    qDebug() << "[MCP BRIDGE] commit called.";
    if (m_dpi) m_dpi->updateView();
}

} // namespace mcp