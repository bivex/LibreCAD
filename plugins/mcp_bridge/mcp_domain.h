#ifndef MCP_DOMAIN_H
#define MCP_DOMAIN_H

#include <QString>
#include <QPointF>
#include <QVector>
#include <QStringList>
#include <QHash>
#include <QVariant>
#include <QJsonArray>
#include <QJsonObject>

namespace mcp {

// --- Core Entities ---
struct Line { QPointF start; QPointF end; };
struct Circle { QPointF center; double radius; };
struct Arc { QPointF center; double radius; double startAngle; double endAngle; };
struct Ellipse { QPointF center; QPointF majorAxisEnd; double ratio; double startAngle; double endAngle; };
struct Polyline { QVector<QPointF> points; bool closed; };
struct Text { QString content; QPointF position; double size; };
struct MText { QString content; QPointF position; double height; double angle; };
struct Point { QPointF position; };
struct SplinePoints { QVector<QPointF> points; bool closed; };
struct ConstructionLine { QPointF p1; QPointF p2; };
struct Solid { QVector<QPointF> points; };

// --- Interior Architecture Entities ---
struct Wall { QPointF start; QPointF end; double thickness; };
struct Opening { QPointF start; QPointF end; QString type; };

// --- Military Entities ---
struct TacticalSymbol { QPointF position; QString identity; QString unitType; double size; };
struct TacticalLine { QVector<QPointF> points; QString type; };

// --- Block/Insert ---
struct BlockInsert {
    QString name;
    QPointF insertionPoint;
    QPointF scale;
    double rotation;
    int rows = 1;
    int cols = 1;
    double rowSpacing = 0.0;
    double colSpacing = 0.0;
};

// --- Dimensions ---
enum class DimType { Linear, Aligned, Radial, Diametric, Angular, Leader };

struct Dimension {
    DimType dimType;
    QPointF start;       // first extension line origin
    QPointF end;         // second extension line origin (or point on circle for radial)
    QPointF dimLinePos;  // dimension line position / angle vertex
    double angle = 0.0;  // rotation angle (for linear)
    QString text;        // override text (empty = auto)
    double arrowSize = 2.5;
};

// --- Layer Properties ---
struct LayerProperties {
    int color = 0;
    QString lineWidth;
    QString lineType;
};

/**
 * @brief Port: Interface for the Drawing Service.
 */
class IDrawingService {
public:
    virtual ~IDrawingService() = default;

    // Core Commands
    virtual void drawLine(const Line& line) = 0;
    virtual void drawConstructionLine(const ConstructionLine& cline) = 0;
    virtual void drawCircle(const Circle& circle) = 0;
    virtual void drawArc(const Arc& arc) = 0;
    virtual void drawEllipse(const Ellipse& ellipse) = 0;
    virtual void drawPolyline(const Polyline& polyline) = 0;
    virtual void drawLines(const Polyline& lines) = 0;
    virtual void drawSolid(const Solid& solid) = 0;
    virtual void addText(const Text& text) = 0;
    virtual void addMText(const MText& mtext) = 0;
    virtual void setLayer(const QString& name) = 0;
    virtual void deleteLayer(const QString& name) = 0;

    // New Core
    virtual void drawPoint(const Point& point) = 0;
    virtual void drawSplinePoints(const SplinePoints& spline) = 0;
    virtual void insertBlock(const BlockInsert& ins) = 0;
    virtual QString addBlockFromDisk(const QString& path) = 0;

    // Dimensions
    virtual void addDimension(const Dimension& dim) = 0;

    // Entity Operations
    virtual bool removeEntity(qulonglong eid) = 0;
    virtual bool moveEntity(qulonglong eid, double dx, double dy, bool copy = false) = 0;
    virtual bool offsetEntity(qulonglong eid, double distance) = 0;
    virtual bool rotateEntity(qulonglong eid, double cx, double cy, double angle, bool copy = false) = 0;
    virtual bool scaleEntity(qulonglong eid, double cx, double cy, double sx, double sy, bool copy = false) = 0;
    virtual bool moveRotateEntity(qulonglong eid, double dx, double dy, double cx, double cy, double angle, bool copy = false) = 0;
    virtual bool updateEntityData(qulonglong eid, const QHash<int, QVariant>& data) = 0;

    // Entity Queries
    virtual QJsonArray getAllEntityData() = 0;
    virtual QJsonObject getEntityDataById(qulonglong eid) = 0;
    virtual QJsonArray getPolylineData(qulonglong eid) = 0;
    virtual bool updatePolylineData(qulonglong eid, const QJsonArray& vertices) = 0;

    // Layer Properties
    virtual LayerProperties getLayerProperties(const QString& layerName) = 0;
    virtual bool setLayerProperties(const QString& layerName, int color, const QString& lineWidth, const QString& lineType) = 0;

    // Domain-Specific Commands
    virtual void drawWall(const Wall& wall) = 0;
    virtual void drawOpening(const Opening& opening) = 0;
    virtual void drawTacticalSymbol(const TacticalSymbol& sym) = 0;
    virtual void drawTacticalLine(const TacticalLine& line) = 0;

    // Queries
    virtual QStringList getLayers() = 0;
    virtual QStringList getBlocks() = 0;
    virtual QString getCurrentLayer() = 0;
    virtual void unselectEntities() = 0;
    virtual QString realToStr(double num, int units = 0, int prec = 0) = 0;

    // Interactive Input
    virtual QPointF getPoint(const QString& message = "") = 0;
    virtual qulonglong getEntity(const QString& message = "") = 0;
    virtual QVector<qulonglong> getSelection(const QString& message = "") = 0;
    virtual QString getString(const QString& message, const QString& title) = 0;
    virtual int getInt(const QString& message, const QString& title) = 0;
    virtual double getReal(const QString& message, const QString& title) = 0;

    // Variables
    virtual QVariant getVariable(const QString& key) = 0;
    virtual bool setVariable(const QString& key, const QVariant& value, int code) = 0;

    virtual void commit() = 0;
};

} // namespace mcp

#endif
