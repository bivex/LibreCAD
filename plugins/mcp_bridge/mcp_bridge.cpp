#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <cstdio>
#include "mcp_bridge.h"
#include "librecad_adapter.h"
#include "command_processor.h"

#define LOG_STEP(msg) do { fprintf(stderr, "[MCP BRIDGE] %s\n", msg); fflush(stderr); } while(0)

MCP_Bridge::MCP_Bridge() {
    LOG_STEP("CONSTRUCTOR called.");
}

MCP_Bridge::~MCP_Bridge() {
    LOG_STEP("DESTRUCTOR called.");
}

QString MCP_Bridge::name() const {
    return "MCP Bridge";
}

PluginCapabilities MCP_Bridge::getCapabilities() const {
    LOG_STEP("getCapabilities() START.");
    PluginCapabilities caps;
    caps.menuEntryPoints << PluginMenuLocation("plugins_menu", "Start MCP Bridge", "");
    caps.paintEventPriorities.clear();
    LOG_STEP("getCapabilities() END.");
    return caps;
}

void MCP_Bridge::execComm(Document_Interface* doc, QWidget* parent, QString cmd) {
    LOG_STEP("execComm() triggered!");
    if (!doc) {
        LOG_STEP("Error: doc is NULL.");
        return;
    }

    if (m_dialog) {
        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
        return;
    }

    m_dialog = new MCP_Bridge_Dialog(doc, parent);
    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_dialog, &QObject::destroyed, this, [this]() { m_dialog = nullptr; });
    m_dialog->show();
}

// --- DIALOG ---

MCP_Bridge_Dialog::MCP_Bridge_Dialog(Document_Interface* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_server(nullptr) {

    setWindowTitle("MCP Bridge");
    setMinimumWidth(300);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("MCP Bridge Active\nPort: 12346"));

    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_stopBtn = new QPushButton("Stop", this);
    connect(m_stopBtn, &QPushButton::clicked, this, &QDialog::close);
    btnLayout->addWidget(m_stopBtn);

    m_hideBtn = new QPushButton("Hide", this);
    connect(m_hideBtn, &QPushButton::clicked, this, &MCP_Bridge_Dialog::hideAndKeepRunning);
    btnLayout->addWidget(m_hideBtn);

    layout->addLayout(btnLayout);

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &MCP_Bridge_Dialog::onNewConnection);
    m_server->listen(QHostAddress::Any, 12346);
}

MCP_Bridge_Dialog::~MCP_Bridge_Dialog() {
    if (m_server) m_server->close();
}

void MCP_Bridge_Dialog::hideAndKeepRunning() {
    hide();
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

    QJsonDocument doc = QJsonDocument::fromJson(socket->readAll());
    if (doc.isNull() || !doc.isObject()) return;

    mcp::LibreCadDrawingAdapter adapter(m_doc, this);
    mcp::CommandProcessor processor(adapter);
    QJsonObject response = processor.process(doc.object());
    socket->write(QJsonDocument(response).toJson());
}
