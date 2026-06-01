#include "librecad_adapter.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariant>
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

// --- New Core ---

void LibreCadDrawingAdapter::drawPoint(const Point& point) {
    qDebug() << "[MCP BRIDGE] drawPoint called.";
    QPointF p = point.position;
    if (m_dpi) m_dpi->addPoint(&p);
}

void LibreCadDrawingAdapter::drawSplinePoints(const SplinePoints& spline) {
    qDebug() << "[MCP BRIDGE] drawSplinePoints called.";
    if (!m_dpi) return;
    std::vector<QPointF> pts;
    for (const auto& p : spline.points)
        pts.push_back(p);
    m_dpi->addSplinePoints(pts, spline.closed);
}

void LibreCadDrawingAdapter::insertBlock(const BlockInsert& ins) {
    qDebug() << "[MCP BRIDGE] insertBlock called:" << ins.name;
    if (m_dpi) m_dpi->addInsert(ins.name, ins.insertionPoint, ins.scale, ins.rotation);
}

QString LibreCadDrawingAdapter::addBlockFromDisk(const QString& path) {
    qDebug() << "[MCP BRIDGE] addBlockFromDisk called:" << path;
    if (m_dpi) return m_dpi->addBlockfromFromdisk(path);
    return QString();
}

// --- Dimensions ---

void LibreCadDrawingAdapter::addDimension(const Dimension& dim) {
    qDebug() << "[MCP BRIDGE] addDimension called.";
    if (!m_dpi) return;

    enum DPI::ETYPE etype;
    switch (dim.dimType) {
    case DimType::Linear:    etype = DPI::DIMLINEAR; break;
    case DimType::Aligned:   etype = DPI::DIMALIGNED; break;
    case DimType::Radial:    etype = DPI::DIMRADIAL; break;
    case DimType::Diametric: etype = DPI::DIMDIAMETRIC; break;
    case DimType::Angular:   etype = DPI::DIMANGULAR; break;
    case DimType::Leader:    etype = DPI::DIMLEADER; break;
    default: return;
    }

    Plug_Entity* ent = m_dpi->newEntity(etype);
    if (!ent) return;

    QHash<int, QVariant> data;
    data[DPI::STARTX] = dim.start.x();
    data[DPI::STARTY] = dim.start.y();
    data[DPI::ENDX] = dim.end.x();
    data[DPI::ENDY] = dim.end.y();
    if (!dim.text.isEmpty())
        data[DPI::TEXTCONTENT] = dim.text;

    switch (dim.dimType) {
    case DimType::Linear:
        data[DPI::STARTANGLE] = dim.angle;
        data[DPI::HEIGHT] = dim.dimLinePos.y();
        break;
    case DimType::Aligned:
        data[DPI::HEIGHT] = dim.dimLinePos.y();
        break;
    case DimType::Radial:
    case DimType::Diametric:
        data[DPI::RADIUS] = sqrt(pow(dim.end.x()-dim.start.x(),2) + pow(dim.end.y()-dim.start.y(),2));
        break;
    case DimType::Angular:
        // dimLinePos used as angle vertex
        break;
    case DimType::Leader:
        break;
    default: break;
    }

    ent->updateData(&data);
    m_dpi->addEntity(ent);
}

// --- Entity Operations ---

QJsonObject LibreCadDrawingAdapter::entityToJson(Plug_Entity* ent) {
    QJsonObject obj;
    if (!ent) return obj;

    QHash<int, QVariant> data;
    ent->getData(&data);

    int etype = data.value(DPI::ETYPE).toInt();
    obj["eid"] = QString::number(data.value(DPI::EID).toULongLong());
    obj["type"] = etype;
    obj["layer"] = data.value(DPI::LAYER).toString();
    obj["color"] = data.value(DPI::COLOR).toInt();
    obj["lineType"] = data.value(DPI::LTYPE).toString();
    obj["lineWidth"] = data.value(DPI::LWIDTH).toString();

    // Type-specific data
    switch (etype) {
    case DPI::LINE:
        obj["x1"] = data[DPI::STARTX].toDouble();
        obj["y1"] = data[DPI::STARTY].toDouble();
        obj["x2"] = data[DPI::ENDX].toDouble();
        obj["y2"] = data[DPI::ENDY].toDouble();
        break;
    case DPI::CIRCLE:
        obj["cx"] = data[DPI::STARTX].toDouble();
        obj["cy"] = data[DPI::STARTY].toDouble();
        obj["r"] = data[DPI::RADIUS].toDouble();
        break;
    case DPI::ARC:
        obj["cx"] = data[DPI::STARTX].toDouble();
        obj["cy"] = data[DPI::STARTY].toDouble();
        obj["r"] = data[DPI::RADIUS].toDouble();
        obj["a1"] = data[DPI::STARTANGLE].toDouble();
        obj["a2"] = data[DPI::ENDANGLE].toDouble();
        break;
    case DPI::ELLIPSE:
        obj["cx"] = data[DPI::STARTX].toDouble();
        obj["cy"] = data[DPI::STARTY].toDouble();
        obj["ex"] = data[DPI::ENDX].toDouble();
        obj["ey"] = data[DPI::ENDY].toDouble();
        obj["ratio"] = data[DPI::HEIGHT].toDouble();
        break;
    case DPI::POINT:
        obj["x"] = data[DPI::STARTX].toDouble();
        obj["y"] = data[DPI::STARTY].toDouble();
        break;
    case DPI::TEXT:
    case DPI::MTEXT:
        obj["text"] = data[DPI::TEXTCONTENT].toString();
        obj["x"] = data[DPI::STARTX].toDouble();
        obj["y"] = data[DPI::STARTY].toDouble();
        obj["height"] = data[DPI::HEIGHT].toDouble();
        obj["angle"] = data[DPI::STARTANGLE].toDouble();
        break;
    case DPI::INSERT:
        obj["blockName"] = data[DPI::BLKNAME].toString();
        obj["x"] = data[DPI::STARTX].toDouble();
        obj["y"] = data[DPI::STARTY].toDouble();
        obj["scaleX"] = data[DPI::XSCALE].toDouble();
        obj["scaleY"] = data[DPI::YSCALE].toDouble();
        obj["angle"] = data[DPI::STARTANGLE].toDouble();
        break;
    default:
        obj["x1"] = data[DPI::STARTX].toDouble();
        obj["y1"] = data[DPI::STARTY].toDouble();
        obj["x2"] = data[DPI::ENDX].toDouble();
        obj["y2"] = data[DPI::ENDY].toDouble();
        break;
    }

    // Type name mapping
    static const char* typeNames[] = {
        "point", "line", "constructionline", "circle", "arc",
        "ellipse", "image", "overlaybox", "solid", "mtext",
        "text", "insert", "polyline", "spline", "splinepoints",
        "hatch", "dimleader", "dimaligned", "dimlinear", "dimradial",
        "dimdiametric", "dimangular", "dimordinate", "tolerance", "unknown"
    };
    if (etype >= 0 && etype < 25)
        obj["typeName"] = typeNames[etype];

    return obj;
}

QJsonArray LibreCadDrawingAdapter::getAllEntityData() {
    qDebug() << "[MCP BRIDGE] getAllEntityData called.";
    QJsonArray arr;
    if (!m_dpi) return arr;

    QList<Plug_Entity*> entities;
    if (m_dpi->getAllEntities(&entities, false)) {
        for (auto* ent : entities) {
            arr.append(entityToJson(ent));
            delete ent;
        }
    }
    return arr;
}

QJsonObject LibreCadDrawingAdapter::getEntityDataById(qulonglong eid) {
    qDebug() << "[MCP BRIDGE] getEntityDataById called:" << eid;
    if (!m_dpi) return QJsonObject();

    QList<Plug_Entity*> entities;
    QJsonObject result;
    if (m_dpi->getAllEntities(&entities, false)) {
        for (auto* ent : entities) {
            QHash<int, QVariant> data;
            ent->getData(&data);
            if (data.value(DPI::EID).toULongLong() == eid) {
                result = entityToJson(ent);
                delete ent;
                // clean up rest
                break;
            }
            delete ent;
        }
    }
    return result;
}

bool LibreCadDrawingAdapter::removeEntity(qulonglong eid) {
    qDebug() << "[MCP BRIDGE] removeEntity called:" << eid;
    if (!m_dpi) return false;

    QList<Plug_Entity*> entities;
    if (!m_dpi->getAllEntities(&entities, false)) return false;

    for (auto* ent : entities) {
        QHash<int, QVariant> data;
        ent->getData(&data);
        if (data.value(DPI::EID).toULongLong() == eid) {
            m_dpi->removeEntity(ent);
            delete ent;
            // Can't safely continue iterating after removal
            // Delete remaining entities in list
            return true;
        }
        delete ent;
    }
    return false;
}

bool LibreCadDrawingAdapter::moveEntity(qulonglong eid, double dx, double dy) {
    qDebug() << "[MCP BRIDGE] moveEntity called:" << eid;
    if (!m_dpi) return false;

    QList<Plug_Entity*> entities;
    if (!m_dpi->getAllEntities(&entities, false)) return false;

    for (auto* ent : entities) {
        QHash<int, QVariant> data;
        ent->getData(&data);
        if (data.value(DPI::EID).toULongLong() == eid) {
            ent->move(QPointF(dx, dy));
            delete ent;
            return true;
        }
        delete ent;
    }
    return false;
}

bool LibreCadDrawingAdapter::rotateEntity(qulonglong eid, double cx, double cy, double angle) {
    qDebug() << "[MCP BRIDGE] rotateEntity called:" << eid;
    if (!m_dpi) return false;

    QList<Plug_Entity*> entities;
    if (!m_dpi->getAllEntities(&entities, false)) return false;

    for (auto* ent : entities) {
        QHash<int, QVariant> data;
        ent->getData(&data);
        if (data.value(DPI::EID).toULongLong() == eid) {
            ent->rotate(QPointF(cx, cy), angle);
            delete ent;
            return true;
        }
        delete ent;
    }
    return false;
}

bool LibreCadDrawingAdapter::scaleEntity(qulonglong eid, double cx, double cy, double sx, double sy) {
    qDebug() << "[MCP BRIDGE] scaleEntity called:" << eid;
    if (!m_dpi) return false;

    QList<Plug_Entity*> entities;
    if (!m_dpi->getAllEntities(&entities, false)) return false;

    for (auto* ent : entities) {
        QHash<int, QVariant> data;
        ent->getData(&data);
        if (data.value(DPI::EID).toULongLong() == eid) {
            ent->scale(QPointF(cx, cy), QPointF(sx, sy));
            delete ent;
            return true;
        }
        delete ent;
    }
    return false;
}

// --- Layer Properties ---

LayerProperties LibreCadDrawingAdapter::getLayerProperties(const QString& layerName) {
    qDebug() << "[MCP BRIDGE] getLayerProperties called:" << layerName;
    LayerProperties props;
    if (!m_dpi) return props;

    QString oldLayer = m_dpi->getCurrentLayer();
    m_dpi->setLayer(layerName);

    int c; QString w, t;
    m_dpi->getCurrentLayerProperties(&c, &w, &t);
    props.color = c;
    props.lineWidth = w;
    props.lineType = t;

    m_dpi->setLayer(oldLayer);
    return props;
}

bool LibreCadDrawingAdapter::setLayerProperties(const QString& layerName, int color, const QString& lineWidth, const QString& lineType) {
    qDebug() << "[MCP BRIDGE] setLayerProperties called:" << layerName;
    if (!m_dpi) return false;

    QString oldLayer = m_dpi->getCurrentLayer();
    m_dpi->setLayer(layerName);
    m_dpi->setCurrentLayerProperties(color, lineWidth, lineType);
    m_dpi->setLayer(oldLayer);
    return true;
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

// --- Variables ---

QVariant LibreCadDrawingAdapter::getVariable(const QString& key) {
    qDebug() << "[MCP BRIDGE] getVariable called:" << key;
    if (!m_dpi) return QVariant();

    int intVal;
    double dblVal;
    if (m_dpi->getVariableInt(key, &intVal))
        return intVal;
    if (m_dpi->getVariableDouble(key, &dblVal))
        return dblVal;
    return QVariant();
}

bool LibreCadDrawingAdapter::setVariable(const QString& key, const QVariant& value, int code) {
    qDebug() << "[MCP BRIDGE] setVariable called:" << key;
    if (!m_dpi) return false;

    if (value.type() == QVariant::Int || value.type() == QVariant::LongLong)
        return m_dpi->addVariable(key, value.toInt(), code);
    else
        return m_dpi->addVariable(key, value.toDouble(), code);
}

void LibreCadDrawingAdapter::commit() {
    // updateView() can crash if called from socket callback context
}

} // namespace mcp
