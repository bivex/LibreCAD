#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <QJsonObject>
#include "mcp_domain.h"

namespace mcp {

class CommandProcessor {
public:
    explicit CommandProcessor(IDrawingService& service) : m_service(service) {}

    QJsonObject process(const QJsonObject& json);

private:
    IDrawingService& m_service;
};

} // namespace mcp

#endif
