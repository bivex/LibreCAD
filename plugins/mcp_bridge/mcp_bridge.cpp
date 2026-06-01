#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QMessageBox>
#include "mcp_bridge.h"
#include "librecad_adapter.h"
#include "command_processor.h"

MCP_Bridge::MCP_Bridge() : m_server(nullptr), m_parent(nullptr), m_doc(nullptr) {}

MCP_Bridge::~MCP_Bridge() {
    if (m_server) {
        m_server->close();
        delete m_server;
    }
}

PluginCapabilities MCP_Bridge::getCapabilities() const {
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
        << PluginMenuLocation("plugins_menu", tr("Start MCP Bridge"),
            tr("Start the Python MCP Bridge server (Clean Architecture)"));
    return pluginCapabilities;
}

void MCP_Bridge::execComm(Document_Interface* doc, QWidget* parent, QString /*cmd*/) {
    m_doc = doc;
    m_parent = parent;

    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &MCP_Bridge::onNewConnection);
        
        const int port = 12346; 
        
        if (!m_server->listen(QHostAddress::Any, port)) {
            QMessageBox::critical(parent, "MCP Bridge", tr("Failed to start server: %1").arg(m_server->errorString()));
            delete m_server;
            m_server = nullptr;
            return;
        }
        QMessageBox::information(parent, "MCP Bridge", tr("MCP Bridge server started on port %1").arg(port));
    } else {
        QMessageBox::information(parent, "MCP Bridge", tr("MCP Bridge server is already running."));
    }
}

void MCP_Bridge::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &MCP_Bridge::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &MCP_Bridge::onDisconnected);
    }
}

void MCP_Bridge::onDisconnected() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) socket->deleteLater();
}

void MCP_Bridge::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return;

    if (!m_doc) return; // No document to draw on
    
    mcp::LibreCadDrawingAdapter adapter(m_doc, m_parent);
    mcp::CommandProcessor processor(adapter);
    
    QJsonObject response = processor.process(doc.object());
    
    socket->write(QJsonDocument(response).toJson());
}
