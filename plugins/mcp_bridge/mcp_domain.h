#ifndef MCP_DOMAIN_H
#define MCP_DOMAIN_H

#include <QString>
#include <QPointF>
#include <QVector>
#include <QStringList>

namespace mcp {

// --- Core Entities ---
struct Line { QPointF start; QPointF end; };
struct Circle { QPointF center; double radius; };
struct Arc { QPointF center; double radius; double startAngle; double endAngle; };
struct Ellipse { QPointF center; QPointF majorAxisEnd; double ratio; double startAngle; double endAngle; };
struct Polyline { QVector<QPointF> points; bool closed; };
struct Text { QString content; QPointF position; double size; };

// --- Interior Architecture Entities ---
struct Wall { QPointF start; QPointF end; double thickness; };
struct Opening { QPointF start; QPointF end; QString type; };

// --- Military Entities ---
struct TacticalSymbol { QPointF position; QString identity; QString unitType; double size; };
struct TacticalLine { QVector<QPointF> points; QString type; };

/**
 * @brief Port: Interface for the Drawing Service.
 */
class IDrawingService {
public:
    virtual ~IDrawingService() = default;
    
    // Core Commands
    virtual void drawLine(const Line& line) = 0;
    virtual void drawCircle(const Circle& circle) = 0;
    virtual void drawArc(const Arc& arc) = 0;
    virtual void drawEllipse(const Ellipse& ellipse) = 0;
    virtual void drawPolyline(const Polyline& polyline) = 0;
    virtual void addText(const Text& text) = 0;
    virtual void setLayer(const QString& name) = 0;
    virtual void deleteLayer(const QString& name) = 0;
    
    // Domain-Specific Commands
    virtual void drawWall(const Wall& wall) = 0;
    virtual void drawOpening(const Opening& opening) = 0;
    virtual void drawTacticalSymbol(const TacticalSymbol& sym) = 0;
    virtual void drawTacticalLine(const TacticalLine& line) = 0;
    
    // Queries
    virtual QStringList getLayers() = 0;
    virtual QStringList getBlocks() = 0;
    virtual QString getCurrentLayer() = 0;

    virtual void commit() = 0;
};

} // namespace mcp

#endif