#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include "mcp_bridge.h"
#include "librecad_adapter.h"
#include "command_processor.h"

MCP_Bridge::MCP_Bridge() {}
MCP_Bridge::~MCP_Bridge() {}

PluginCapabilities MCP_Bridge::getCapabilities() const {
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
        << PluginMenuLocation("plugins_menu", tr("Start MCP Bridge"),
            tr("Start the Python MCP Bridge server (Persistent)"));
    return pluginCapabilities;
}

void MCP_Bridge::execComm(Document_Interface* doc, QWidget* parent, QString /*cmd*/) {
    MCP_Bridge_Dialog dlg(doc, parent);
    dlg.exec(); // This nested event loop keeps doc (on the stack of the caller) alive.
}

MCP_Bridge_Dialog::MCP_Bridge_Dialog(Document_Interface* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_server(nullptr) {
    
    setWindowTitle("MCP Bridge");
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("MCP Bridge is running on port 12346.\nKeep this window open to process commands."));
    
    QPushButton* closeBtn = new QPushButton("Stop Bridge", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &MCP_Bridge_Dialog::onNewConnection);
    
    if (!m_server->listen(QHostAddress::Any, 12346)) {
        QMessageBox::critical(this, "Error", "Failed to start server: " + m_server->errorString());
    }
}

MCP_Bridge_Dialog::~MCP_Bridge_Dialog() {
    if (m_server) m_server->close();
}

void MCP_Bridge_Dialog::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &MCP_Bridge_Dialog::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &MCP_Bridge_Dialog::onDisconnected);
    }
}

void MCP_Bridge_Dialog::onDisconnected() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) socket->deleteLater();
}

void MCP_Bridge_Dialog::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return;

    mcp::LibreCadDrawingAdapter adapter(m_doc, this);
    mcp::CommandProcessor processor(adapter);
    
    QJsonObject response = processor.process(doc.object());
    
    socket->write(QJsonDocument(response).toJson());
}
