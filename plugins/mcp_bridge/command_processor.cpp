#include "command_processor.h"
#include <QJsonArray>
#include <QDebug>

namespace mcp {

QJsonObject CommandProcessor::process(const QJsonObject& json) {
    QString method = json["method"].toString();
    qDebug() << "[MCP BRIDGE] CommandProcessor handling method:" << method;

    QJsonObject params = json["params"].toObject();
    QJsonObject response;
    response["status"] = "ok";

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
        QVector<QPointF> points;
        for (int i = 0; i < pointsArr.size(); ++i) {
            QJsonObject p = pointsArr[i].toObject();
            points.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));
        }
        m_service.drawPolyline({points, params["closed"].toBool(false)});
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
        for(int i=0; i<pts.size(); ++i) points.append({pts[i].toObject()["x"].toDouble(), pts[i].toObject()["y"].toDouble()});
        m_service.drawTacticalLine({points, params["type"].toString("boundary")});
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
    } else {
        response["status"] = "error";
        response["message"] = "Unknown method: " + method;
        qDebug() << "[MCP BRIDGE] Error: Unknown method" << method;
    }

    m_service.commit();
    return response;
}

} // namespace mcp