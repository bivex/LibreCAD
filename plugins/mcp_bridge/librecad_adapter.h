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
    void drawConstructionLine(const ConstructionLine& cline) override;
    void drawCircle(const Circle& circle) override;
    void drawArc(const Arc& arc) override;
    void drawEllipse(const Ellipse& ellipse) override;
    void drawPolyline(const Polyline& polyline) override;
    void drawLines(const Polyline& lines) override;
    void drawSolid(const Solid& solid) override;
    void addText(const Text& text) override;
    void addMText(const MText& mtext) override;
    void setLayer(const QString& name) override;
    void deleteLayer(const QString& name) override;

    // New Core
    void drawPoint(const Point& point) override;
    void drawSplinePoints(const SplinePoints& spline) override;
    void insertBlock(const BlockInsert& ins) override;
    QString addBlockFromDisk(const QString& path) override;

    // Dimensions
    void addDimension(const Dimension& dim) override;

    // Entity Operations
    bool removeEntity(qulonglong eid) override;
    bool moveEntity(qulonglong eid, double dx, double dy, bool copy = false) override;
    bool offsetEntity(qulonglong eid, double distance) override;
    bool rotateEntity(qulonglong eid, double cx, double cy, double angle, bool copy = false) override;
    bool scaleEntity(qulonglong eid, double cx, double cy, double sx, double sy, bool copy = false) override;
    bool moveRotateEntity(qulonglong eid, double dx, double dy, double cx, double cy, double angle, bool copy = false) override;
    bool updateEntityData(qulonglong eid, const QHash<int, QVariant>& data) override;

    // Entity Queries
    QJsonArray getAllEntityData() override;
    QJsonObject getEntityDataById(qulonglong eid) override;
    QJsonArray getPolylineData(qulonglong eid) override;
    bool updatePolylineData(qulonglong eid, const QJsonArray& vertices) override;

    // Layer Properties
    LayerProperties getLayerProperties(const QString& layerName) override;
    bool setLayerProperties(const QString& layerName, int color, const QString& lineWidth, const QString& lineType) override;

    // Domain-Specific Commands
    void drawWall(const Wall& wall) override;
    void drawOpening(const Opening& opening) override;
    void drawTacticalSymbol(const TacticalSymbol& sym) override;
    void drawTacticalLine(const TacticalLine& line) override;

    // Queries
    QStringList getLayers() override;
    QStringList getBlocks() override;
    QString getCurrentLayer() override;
    void unselectEntities() override;
    QString realToStr(double num, int units = 0, int prec = 0) override;

    // Interactive Input
    QPointF getPoint(const QString& message = "") override;
    qulonglong getEntity(const QString& message = "") override;
    QVector<qulonglong> getSelection(const QString& message = "") override;
    QString getString(const QString& message, const QString& title) override;
    int getInt(const QString& message, const QString& title) override;
    double getReal(const QString& message, const QString& title) override;

    // Variables
    QVariant getVariable(const QString& key) override;
    bool setVariable(const QString& key, const QVariant& value, int code) override;

    void commit() override;

private:
    Document_Interface* m_dpi;
    QWidget* m_parent;

    QJsonObject entityToJson(Plug_Entity* ent);
};

} // namespace mcp

#endif
