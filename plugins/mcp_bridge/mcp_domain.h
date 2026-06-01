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
struct Text { QString content; QPointF position; double size; };

// --- Interior Architecture Entities ---
struct Wall {
    QPointF start;
    QPointF end;
    double thickness;
};

struct Opening { // Door or Window
    QPointF start;
    QPointF end;
    QString type; // "door", "window"
};

// --- Military Entities (NATO APP-6 inspired) ---
struct TacticalSymbol {
    QPointF position;
    QString identity; // "hostile", "friendly", "neutral"
    QString unitType; // "infantry", "armor", "artillery"
    double size;
};

struct TacticalLine {
    QVector<QPointF> points;
    QString type; // "boundary", "axis_of_advance", "main_attack"
};

/**
 * @brief Port: Interface for the Drawing Service.
 */
class IDrawingService {
public:
    virtual ~IDrawingService() = default;
    
    // Core
    virtual void drawLine(const Line& line) = 0;
    virtual void drawCircle(const Circle& circle) = 0;
    virtual void addText(const Text& text) = 0;
    virtual void setLayer(const QString& name) = 0;
    
    // Interior Tools
    virtual void drawWall(const Wall& wall) = 0;
    virtual void drawOpening(const Opening& opening) = 0;

    // Military Tools
    virtual void drawTacticalSymbol(const TacticalSymbol& sym) = 0;
    virtual void drawTacticalLine(const TacticalLine& line) = 0;
    
    // Queries
    virtual QStringList getLayers() = 0;
    virtual void commit() = 0;
};

} // namespace mcp

#endif
