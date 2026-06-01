#ifndef LIBRECAD_ADAPTER_H
#define LIBRECAD_ADAPTER_H

#include "mcp_domain.h"
#include "document_interface.h"
#include <QDebug>

namespace mcp {

class LibreCadDrawingAdapter : public IDrawingService {
public:
    LibreCadDrawingAdapter(Document_Interface* dpi, QWidget* parent);

    // Core Commands
    void drawLine(const Line& line) override;
    void drawCircle(const Circle& circle) override;
    void drawArc(const Arc& arc) override;
    void drawEllipse(const Ellipse& ellipse) override;
    void drawPolyline(const Polyline& polyline) override;
    void addText(const Text& text) override;
    void setLayer(const QString& name) override;
    void deleteLayer(const QString& name) override;

    // Domain-Specific Commands
    void drawWall(const Wall& wall) override;
    void drawOpening(const Opening& opening) override;
    void drawTacticalSymbol(const TacticalSymbol& sym) override;
    void drawTacticalLine(const TacticalLine& line) override;

    // Queries
    QStringList getLayers() override;
    QStringList getBlocks() override;
    QString getCurrentLayer() override;

    void commit() override;

private:
    Document_Interface* m_dpi;
    QWidget* m_parent;
};

} // namespace mcp

#endif