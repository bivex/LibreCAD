#include "command_processor.h"
#include <QJsonArray>

namespace mcp {

QJsonObject CommandProcessor::process(const QJsonObject& json) {
    QString method = json["method"].toString();
    QJsonObject params = json["params"].toObject();
    QJsonObject response;
    response["status"] = "ok";

    if (method == "addWall") {
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
    } else if (method == "getLayers") {
        response["layers"] = QJsonArray::fromStringList(m_service.getLayers());
    }

    m_service.commit();
    return response;
}

} // namespace mcp
