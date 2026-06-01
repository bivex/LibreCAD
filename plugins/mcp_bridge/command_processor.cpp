#include "command_processor.h"
#include <QJsonArray>
#include <cmath>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace mcp {

// --- Geometry helpers ---

static double dist(const QPointF& a, const QPointF& b) {
    double dx = b.x() - a.x(), dy = b.y() - a.y();
    return std::sqrt(dx*dx + dy*dy);
}

static double angleDeg(const QPointF& a, const QPointF& b) {
    return std::atan2(b.y() - a.y(), b.x() - a.x()) * 180.0 / M_PI;
}

static QPointF polar(const QPointF& origin, double angleDeg, double length) {
    double rad = angleDeg * M_PI / 180.0;
    return QPointF(origin.x() + length * std::cos(rad), origin.y() + length * std::sin(rad));
}

static QPointF midpoint(const QPointF& a, const QPointF& b) {
    return QPointF((a.x()+b.x())/2.0, (a.y()+b.y())/2.0);
}

static QPointF projectOnLine(const QPointF& p, const QPointF& a, const QPointF& b) {
    QPointF ab(b.x()-a.x(), b.y()-a.y());
    QPointF ap(p.x()-a.x(), p.y()-a.y());
    double t = (ap.x()*ab.x() + ap.y()*ab.y()) / (ab.x()*ab.x() + ab.y()*ab.y());
    return QPointF(a.x() + t*ab.x(), a.y() + t*ab.y());
}

static QPointF normalVec(const QPointF& a, const QPointF& b) {
    double dx = b.x()-a.x(), dy = b.y()-a.y();
    double len = std::sqrt(dx*dx + dy*dy);
    if (len < 1e-9) return QPointF(0, 1);
    return QPointF(-dy/len, dx/len);
}

// --- Command processing ---

QJsonObject CommandProcessor::process(const QJsonObject& json) {
    QString method = json["method"].toString();
    qDebug() << "[MCP BRIDGE] CommandProcessor handling method:" << method;

    QJsonObject params = json["params"].toObject();
    QJsonObject response;
    response["status"] = "ok";

    // ========== Original primitives ==========

    if (method == "addLine") {
        m_service.drawLine({
            {params["x1"].toDouble(), params["y1"].toDouble()},
            {params["x2"].toDouble(), params["y2"].toDouble()}
        });
    } else if (method == "addCircle") {
        m_service.drawCircle({
            {params["x"].toDouble(), params["y"].toDouble()},
            params["r"].toDouble()
        });
    } else if (method == "addArc") {
        m_service.drawArc({
            {params["x"].toDouble(), params["y"].toDouble()},
            params["r"].toDouble(),
            params["a1"].toDouble(),
            params["a2"].toDouble()
        });
    } else if (method == "addEllipse") {
        m_service.drawEllipse({
            {params["cx"].toDouble(), params["cy"].toDouble()},
            {params["ex"].toDouble(), params["ey"].toDouble()},
            params["ratio"].toDouble(0.5),
            params["a1"].toDouble(0.0),
            params["a2"].toDouble(360.0)
        });
    } else if (method == "addPolyline") {
        QJsonArray pointsArr = params["points"].toArray();
        if (pointsArr.size() < 2) {
            response["status"] = "error";
            response["message"] = "addPolyline requires at least 2 points";
        } else {
            QVector<QPointF> points;
            for (int i = 0; i < pointsArr.size(); ++i) {
                QJsonObject p = pointsArr[i].toObject();
                points.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));
            }
            m_service.drawPolyline({points, params["closed"].toBool(false)});
        }

    } else if (method == "addLines") {
        QJsonArray pointsArr = params["points"].toArray();
        if (pointsArr.size() < 2) {
            response["status"] = "error";
            response["message"] = "addLines requires at least 2 points";
        } else {
            QVector<QPointF> points;
            for (int i = 0; i < pointsArr.size(); ++i) {
                QJsonObject p = pointsArr[i].toObject();
                points.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));
            }
            m_service.drawLines({points, params["closed"].toBool(false)});
        }

    } else if (method == "addRectangle") {
        double x = params["x"].toDouble();
        double y = params["y"].toDouble();
        double w = params["w"].toDouble();
        double h = params["h"].toDouble();
        QVector<QPointF> points = {
            {x, y}, {x+w, y}, {x+w, y+h}, {x, y+h}
        };
        m_service.drawPolyline({points, true});
    } else if (method == "addText") {
        m_service.addText({
            params["text"].toString(),
            {params["x"].toDouble(), params["y"].toDouble()},
            params["size"].toDouble(10.0)
        });

    // ========== New Primitives ==========

    } else if (method == "addPoint") {
        m_service.drawPoint({
            {params["x"].toDouble(), params["y"].toDouble()}
        });

    } else if (method == "addSplinePoints") {
        QJsonArray pointsArr = params["points"].toArray();
        if (pointsArr.size() < 2) {
            response["status"] = "error";
            response["message"] = "addSplinePoints requires at least 2 points";
        } else {
            QVector<QPointF> points;
            for (int i = 0; i < pointsArr.size(); ++i) {
                QJsonObject p = pointsArr[i].toObject();
                points.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));
            }
            m_service.drawSplinePoints({points, params["closed"].toBool(false)});
        }

    } else if (method == "addInsert") {
        m_service.insertBlock({
            params["name"].toString(),
            {params["x"].toDouble(), params["y"].toDouble()},
            {params["sx"].toDouble(1.0), params["sy"].toDouble(1.0)},
            params["rotation"].toDouble(0.0)
        });

    } else if (method == "addBlockFromDisk") {
        QString result = m_service.addBlockFromDisk(params["path"].toString());
        if (result.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Failed to load block from disk";
        } else {
            response["blockName"] = result;
        }

    // ========== Dimensions ==========

    } else if (method == "addDimLinear") {
        m_service.addDimension({
            DimType::Linear,
            {params["x1"].toDouble(), params["y1"].toDouble()},
            {params["x2"].toDouble(), params["y2"].toDouble()},
            {0.0, params["dimLineOffset"].toDouble(10.0)},
            params["angle"].toDouble(0.0),
            params["text"].toString(""),
            2.5
        });

    } else if (method == "addDimAligned") {
        m_service.addDimension({
            DimType::Aligned,
            {params["x1"].toDouble(), params["y1"].toDouble()},
            {params["x2"].toDouble(), params["y2"].toDouble()},
            {0.0, params["dimLineOffset"].toDouble(10.0)},
            0.0,
            params["text"].toString(""),
            2.5
        });

    } else if (method == "addDimRadial") {
        m_service.addDimension({
            DimType::Radial,
            {params["cx"].toDouble(), params["cy"].toDouble()},
            {params["ex"].toDouble(), params["ey"].toDouble()},
            {},
            0.0,
            params["text"].toString(""),
            2.5
        });

    } else if (method == "addDimDiametric") {
        m_service.addDimension({
            DimType::Diametric,
            {params["cx"].toDouble(), params["cy"].toDouble()},
            {params["ex"].toDouble(), params["ey"].toDouble()},
            {},
            0.0,
            params["text"].toString(""),
            2.5
        });

    } else if (method == "addDimAngular") {
        m_service.addDimension({
            DimType::Angular,
            {params["x1"].toDouble(), params["y1"].toDouble()},
            {params["x2"].toDouble(), params["y2"].toDouble()},
            {params["vx"].toDouble(), params["vy"].toDouble()},
            0.0,
            params["text"].toString(""),
            2.5
        });

    } else if (method == "addDimLeader") {
        m_service.addDimension({
            DimType::Leader,
            {params["x1"].toDouble(), params["y1"].toDouble()},
            {params["x2"].toDouble(), params["y2"].toDouble()},
            {},
            0.0,
            params["text"].toString(""),
            2.5
        });

    // ========== Entity Queries ==========

    } else if (method == "getAllEntities") {
        response["entities"] = m_service.getAllEntityData();

    } else if (method == "getEntityById") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        QJsonObject data = m_service.getEntityDataById(eid);
        if (data.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Entity not found";
        } else {
            response["entity"] = data;
        }

    // ========== Entity Operations ==========

    } else if (method == "removeEntity") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        if (!m_service.removeEntity(eid)) {
            response["status"] = "error";
            response["message"] = "Entity not found or cannot be removed";
        }

    } else if (method == "moveEntity") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        if (!m_service.moveEntity(eid, params["dx"].toDouble(), params["dy"].toDouble())) {
            response["status"] = "error";
            response["message"] = "Entity not found";
        }

    } else if (method == "rotateEntity") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        if (!m_service.rotateEntity(eid, params["cx"].toDouble(), params["cy"].toDouble(), params["angle"].toDouble())) {
            response["status"] = "error";
            response["message"] = "Entity not found";
        }

    } else if (method == "scaleEntity") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        if (!m_service.scaleEntity(eid, params["cx"].toDouble(), params["cy"].toDouble(),
                                    params["sx"].toDouble(), params["sy"].toDouble())) {
            response["status"] = "error";
            response["message"] = "Entity not found";
        }

    } else if (method == "moveRotateEntity") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        if (!m_service.moveRotateEntity(eid,
                    params["dx"].toDouble(), params["dy"].toDouble(),
                    params["cx"].toDouble(), params["cy"].toDouble(),
                    params["angle"].toDouble())) {
            response["status"] = "error";
            response["message"] = "Entity not found";
        }

    } else if (method == "updateEntity") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        QJsonObject dataObj = params["data"].toObject();
        QHash<int, QVariant> data;
        for (auto it = dataObj.begin(); it != dataObj.end(); ++it) {
            int key = it.key().toInt();
            if (it.value().isString())
                data[key] = it.value().toString();
            else if (it.value().isDouble())
                data[key] = it.value().toDouble();
            else
                data[key] = it.value().toVariant();
        }
        if (!m_service.updateEntityData(eid, data)) {
            response["status"] = "error";
            response["message"] = "Entity not found";
        }

    } else if (method == "getPolylineData") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        QJsonArray verts = m_service.getPolylineData(eid);
        if (verts.isEmpty()) {
            response["status"] = "error";
            response["message"] = "Polyline not found or has no vertices";
        } else {
            response["vertices"] = verts;
        }

    } else if (method == "updatePolylineData") {
        qulonglong eid = QString::number(params["eid"].toDouble()).toULongLong();
        QJsonArray verts = params["vertices"].toArray();
        if (!m_service.updatePolylineData(eid, verts)) {
            response["status"] = "error";
            response["message"] = "Polyline not found";
        }

    // ========== Layer Properties ==========

    } else if (method == "getLayerProperties") {
        LayerProperties props = m_service.getLayerProperties(params["layer"].toString());
        response["color"] = props.color;
        response["lineWidth"] = props.lineWidth;
        response["lineType"] = props.lineType;

    } else if (method == "setLayerProperties") {
        if (!m_service.setLayerProperties(
                params["layer"].toString(),
                params["color"].toInt(0),
                params["lineWidth"].toString("0.00mm"),
                params["lineType"].toString("SolidLine"))) {
            response["status"] = "error";
            response["message"] = "Failed to set layer properties";
        }

    // ========== Variables ==========

    } else if (method == "getVariable") {
        QVariant val = m_service.getVariable(params["key"].toString());
        if (val.isValid()) {
            if (val.type() == QVariant::Int)
                response["value"] = val.toInt();
            else
                response["value"] = val.toDouble();
        } else {
            response["status"] = "error";
            response["message"] = "Variable not found";
        }

    } else if (method == "setVariable") {
        int code = params["code"].toInt(70);
        if (!m_service.setVariable(params["key"].toString(), params["value"].toVariant(), code)) {
            response["status"] = "error";
            response["message"] = "Failed to set variable";
        }

    } else if (method == "unselectEntities") {
        m_service.unselectEntities();

    } else if (method == "realToStr") {
        QString str = m_service.realToStr(
            params["num"].toDouble(),
            params["units"].toInt(0),
            params["prec"].toInt(0)
        );
        response["result"] = str;

    // ========== Architectural ==========

    } else if (method == "addWall") {
        m_service.drawWall({
            {params["x1"].toDouble(), params["y1"].toDouble()},
            {params["x2"].toDouble(), params["y2"].toDouble()},
            params["thickness"].toDouble(0.2)
        });
    } else if (method == "addOpening") {
        m_service.drawOpening({
            {params["x1"].toDouble(), params["y1"].toDouble()},
            {params["x2"].toDouble(), params["y2"].toDouble()},
            params["type"].toString("door")
        });

    // ========== Tactical ==========

    } else if (method == "addTacticalSymbol") {
        m_service.drawTacticalSymbol({
            {params["x"].toDouble(), params["y"].toDouble()},
            params["identity"].toString("friendly"),
            params["unit"].toString("infantry"),
            params["size"].toDouble(5.0)
        });
    } else if (method == "addTacticalLine") {
        QJsonArray pts = params["points"].toArray();
        QVector<QPointF> points;
        for(int i=0; i<pts.size(); ++i)
            points.append({pts[i].toObject()["x"].toDouble(), pts[i].toObject()["y"].toDouble()});
        m_service.drawTacticalLine({points, params["type"].toString("boundary")});

    // ========== Layers / Blocks ==========

    } else if (method == "setLayer") {
        m_service.setLayer(params["name"].toString());
    } else if (method == "deleteLayer") {
        m_service.deleteLayer(params["name"].toString());
    } else if (method == "getLayers") {
        response["layers"] = QJsonArray::fromStringList(m_service.getLayers());
    } else if (method == "getBlocks") {
        response["blocks"] = QJsonArray::fromStringList(m_service.getBlocks());
    } else if (method == "getCurrentLayer") {
        response["layer"] = m_service.getCurrentLayer();

    // ========== Construction: Line Tools ==========

    } else if (method == "line2Points") {
        QPointF p1(params["x1"].toDouble(), params["y1"].toDouble());
        QPointF p2(params["x2"].toDouble(), params["y2"].toDouble());
        m_service.drawLine({p1, p2});

    } else if (method == "lineAngle") {
        QPointF origin(params["x"].toDouble(), params["y"].toDouble());
        double angle = params["angle"].toDouble();
        double length = params["length"].toDouble(10.0);
        m_service.drawLine({origin, polar(origin, angle, length)});

    } else if (method == "lineHorizontal") {
        double x = params["x"].toDouble();
        double y = params["y"].toDouble();
        double length = params["length"].toDouble(10.0);
        m_service.drawLine({QPointF(x, y), QPointF(x + length, y)});

    } else if (method == "lineVertical") {
        double x = params["x"].toDouble();
        double y = params["y"].toDouble();
        double length = params["length"].toDouble(10.0);
        m_service.drawLine({QPointF(x, y), QPointF(x, y + length)});

    } else if (method == "lineParallelThroughPoint") {
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        QPointF p(params["px"].toDouble(), params["py"].toDouble());
        double len = dist(a, b);
        if (len < 1e-9) { response["status"] = "error"; response["message"] = "Zero-length reference line"; }
        else {
            double ang = angleDeg(a, b);
            m_service.drawLine({p, polar(p, ang, len)});
        }

    } else if (method == "lineParallel") {
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        double d = params["distance"].toDouble();
        QPointF n = normalVec(a, b);
        QPointF offset(n.x() * d, n.y() * d);
        m_service.drawLine({QPointF(a.x()+offset.x(), a.y()+offset.y()),
                            QPointF(b.x()+offset.x(), b.y()+offset.y())});

    } else if (method == "lineBisector") {
        QPointF v(params["vx"].toDouble(), params["vy"].toDouble());
        QPointF a(params["ax"].toDouble(), params["ay"].toDouble());
        QPointF b(params["bx"].toDouble(), params["by"].toDouble());
        double length = params["length"].toDouble(10.0);
        double angA = angleDeg(v, a);
        double angB = angleDeg(v, b);
        double diff = angB - angA;
        while (diff > 180) diff -= 360;
        while (diff < -180) diff += 360;
        double bisectAngle = angA + diff / 2.0;
        m_service.drawLine({v, polar(v, bisectAngle, length)});

    } else if (method == "lineTangentPC") {
        QPointF p(params["px"].toDouble(), params["py"].toDouble());
        QPointF c(params["cx"].toDouble(), params["cy"].toDouble());
        double r = params["r"].toDouble();
        double d = dist(p, c);
        if (d <= r) { response["status"] = "error"; response["message"] = "Point is inside or on the circle"; }
        else {
            double ang = angleDeg(p, c);
            double alpha = std::acos(r / d) * 180.0 / M_PI;
            double dpc = dist(p, c);
            double tangentLen = std::sqrt(dpc*dpc - r*r);
            QPointF t1 = polar(p, ang + alpha, tangentLen);
            QPointF t2 = polar(p, ang - alpha, tangentLen);
            m_service.drawLine({p, t1});
            m_service.drawLine({p, t2});
            QJsonArray tangentPts;
            tangentPts.append(QJsonObject{{"x", t1.x()}, {"y", t1.y()}});
            tangentPts.append(QJsonObject{{"x", t2.x()}, {"y", t2.y()}});
            response["tangentPoints"] = tangentPts;
        }

    } else if (method == "lineTangentCC") {
        QPointF c1(params["cx1"].toDouble(), params["cy1"].toDouble());
        double r1 = params["r1"].toDouble();
        QPointF c2(params["cx2"].toDouble(), params["cy2"].toDouble());
        double r2 = params["r2"].toDouble();
        double d = dist(c1, c2);
        if (d < std::fabs(r1 - r2) + 1e-9) {
            response["status"] = "error"; response["message"] = "One circle is inside the other";
        } else {
            double ang = angleDeg(c1, c2);
            double dr = r1 - r2;
            double alpha = std::asin(dr / d) * 180.0 / M_PI;
            double a1 = ang + 90 - alpha;
            QPointF s1 = polar(c1, a1 - 90, r1);
            QPointF e1 = polar(c2, a1 - 90, r2);
            m_service.drawLine({s1, e1});
            double a2 = ang - 90 + alpha;
            QPointF s2 = polar(c1, a2 + 90, r1);
            QPointF e2 = polar(c2, a2 + 90, r2);
            m_service.drawLine({s2, e2});
        }

    } else if (method == "lineTangentOrthogonal") {
        QPointF c(params["cx"].toDouble(), params["cy"].toDouble());
        double r = params["r"].toDouble();
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        double lineAngle = angleDeg(a, b);
        double perpAngle = lineAngle + 90.0;
        QPointF t1 = polar(c, perpAngle, r);
        QPointF t2 = polar(c, perpAngle + 180.0, r);
        QPointF n = normalVec(a, b);
        double ext = params["length"].toDouble(10.0);
        m_service.drawLine({QPointF(t1.x() - n.x()*ext, t1.y() - n.y()*ext),
                            QPointF(t1.x() + n.x()*ext, t1.y() + n.y()*ext)});
        m_service.drawLine({QPointF(t2.x() - n.x()*ext, t2.y() - n.y()*ext),
                            QPointF(t2.x() + n.x()*ext, t2.y() + n.y()*ext)});

    } else if (method == "lineOrthogonal") {
        QPointF p(params["px"].toDouble(), params["py"].toDouble());
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        QPointF proj = projectOnLine(p, a, b);
        m_service.drawLine({p, proj});
        response["footX"] = proj.x();
        response["footY"] = proj.y();

    } else if (method == "lineRelativeAngle") {
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        QPointF p(params["px"].toDouble(), params["py"].toDouble());
        double relAngle = params["angle"].toDouble();
        double length = params["length"].toDouble(10.0);
        double baseAngle = angleDeg(a, b);
        m_service.drawLine({p, polar(p, baseAngle + relAngle, length)});

    } else if (method == "lineSnake") {
        QJsonArray pts = params["points"].toArray();
        QVector<QPointF> points;
        for (int i = 0; i < pts.size(); ++i) {
            QJsonObject p = pts[i].toObject();
            points.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));
        }
        for (int i = 0; i < points.size() - 1; ++i)
            m_service.drawLine({points[i], points[i+1]});

    } else if (method == "lineSnakeX") {
        double x = params["x"].toDouble();
        double y = params["y"].toDouble();
        double width = params["width"].toDouble(10.0);
        int segments = params["segments"].toInt(3);
        double gap = params["gap"].toDouble(3.0);
        for (int i = 0; i < segments; ++i) {
            double yi = y + i * gap;
            double xe = (i % 2 == 0) ? x + width : x;
            double xs = (i % 2 == 0) ? x : x + width;
            m_service.drawLine({QPointF(xs, yi), QPointF(xe, yi)});
            if (i < segments - 1)
                m_service.drawLine({QPointF(xe, yi), QPointF(xe, yi + gap)});
        }

    } else if (method == "lineSnakeY") {
        double x = params["x"].toDouble();
        double y = params["y"].toDouble();
        double height = params["height"].toDouble(10.0);
        int segments = params["segments"].toInt(3);
        double gap = params["gap"].toDouble(3.0);
        for (int i = 0; i < segments; ++i) {
            double xi = x + i * gap;
            double ye = (i % 2 == 0) ? y + height : y;
            double ys = (i % 2 == 0) ? y : y + height;
            m_service.drawLine({QPointF(xi, ys), QPointF(xi, ye)});
            if (i < segments - 1)
                m_service.drawLine({QPointF(xi, ye), QPointF(xi + gap, ye)});
        }

    } else if (method == "lineAngleFromLine") {
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        double offsetRatio = params["t"].toDouble(0.5);
        double angOffset = params["angle"].toDouble();
        double length = params["length"].toDouble(10.0);
        QPointF start(a.x() + offsetRatio*(b.x()-a.x()), a.y() + offsetRatio*(b.y()-a.y()));
        double baseAngle = angleDeg(a, b);
        m_service.drawLine({start, polar(start, baseAngle + angOffset, length)});

    } else if (method == "lineOrthogonalFromLine") {
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        double offsetRatio = params["t"].toDouble(0.5);
        double length = params["length"].toDouble(10.0);
        QPointF start(a.x() + offsetRatio*(b.x()-a.x()), a.y() + offsetRatio*(b.y()-a.y()));
        double lineAngle = angleDeg(a, b);
        m_service.drawLine({start, polar(start, lineAngle + 90.0, length)});

    } else if (method == "lineFromPointToLine") {
        QPointF p(params["px"].toDouble(), params["py"].toDouble());
        QPointF a(params["lx1"].toDouble(), params["ly1"].toDouble());
        QPointF b(params["lx2"].toDouble(), params["ly2"].toDouble());
        QPointF proj = projectOnLine(p, a, b);
        m_service.drawLine({p, proj});
        response["footX"] = proj.x();
        response["footY"] = proj.y();

    // ========== Construction: Slice/Divide ==========

    } else if (method == "sliceDivideLine") {
        QPointF a(params["x1"].toDouble(), params["y1"].toDouble());
        QPointF b(params["x2"].toDouble(), params["y2"].toDouble());
        int n = params["segments"].toInt(2);
        if (n < 1) n = 1;
        QPointF nVec = normalVec(a, b);
        double markSize = params["markSize"].toDouble(0.5);
        QJsonArray divisionPts;
        for (int i = 0; i <= n; ++i) {
            double t = (double)i / n;
            QPointF pt(a.x() + t*(b.x()-a.x()), a.y() + t*(b.y()-a.y()));
            m_service.drawLine({QPointF(pt.x() - nVec.x()*markSize, pt.y() - nVec.y()*markSize),
                                QPointF(pt.x() + nVec.x()*markSize, pt.y() + nVec.y()*markSize)});
            divisionPts.append(QJsonObject{{"x", pt.x()}, {"y", pt.y()}});
        }
        response["points"] = divisionPts;

    } else if (method == "sliceDivideCircle") {
        QPointF c(params["cx"].toDouble(), params["cy"].toDouble());
        double r = params["r"].toDouble();
        int n = params["segments"].toInt(4);
        if (n < 1) n = 1;
        double markSize = params["markSize"].toDouble(0.3);
        QJsonArray divisionPts;
        for (int i = 0; i < n; ++i) {
            double ang = 360.0 * i / n;
            QPointF pt = polar(c, ang, r);
            m_service.drawLine({QPointF(pt.x() - markSize, pt.y()), QPointF(pt.x() + markSize, pt.y())});
            m_service.drawLine({QPointF(pt.x(), pt.y() - markSize), QPointF(pt.x(), pt.y() + markSize)});
            divisionPts.append(QJsonObject{{"x", pt.x()}, {"y", pt.y()}, {"angle", ang}});
        }
        response["points"] = divisionPts;

    // ========== Construction: Center Marks ==========

    } else if (method == "centerMark") {
        QPointF c(params["cx"].toDouble(), params["cy"].toDouble());
        double size = params["size"].toDouble(1.0);
        m_service.drawLine({QPointF(c.x() - size, c.y()), QPointF(c.x() + size, c.y())});
        m_service.drawLine({QPointF(c.x(), c.y() - size), QPointF(c.x(), c.y() + size)});

    } else if (method == "centerline") {
        QPointF c(params["cx"].toDouble(), params["cy"].toDouble());
        double r = params["r"].toDouble();
        double ext = params["extension"].toDouble(2.0);
        double total = r + ext;
        m_service.drawLine({QPointF(c.x() - total, c.y()), QPointF(c.x() + total, c.y())});
        m_service.drawLine({QPointF(c.x(), c.y() - total), QPointF(c.x(), c.y() + total)});

    // ========== Unknown ==========

    } else {
        response["status"] = "error";
        response["message"] = "Unknown method: " + method;
        qDebug() << "[MCP BRIDGE] Error: Unknown method" << method;
    }

    m_service.commit();
    return response;
}

} // namespace mcp
