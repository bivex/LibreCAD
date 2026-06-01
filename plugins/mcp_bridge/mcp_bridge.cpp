#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QEventLoop>
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

    // Non-modal dialog + QEventLoop keeps execComm() on the stack
    // so Doc_plugin_interface stays alive, but the user can still
    // interact with the main LibreCAD window.
    MCP_Bridge_Dialog dlg(doc, parent);
    dlg.show();

    QEventLoop loop;
    connect(&dlg, &QDialog::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

// --- DIALOG ---

MCP_Bridge_Dialog::MCP_Bridge_Dialog(Document_Interface* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_server(nullptr) {

    setWindowTitle("MCP Bridge — Port 12346");
    setMinimumWidth(300);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("MCP Bridge Active\nListening on port 12346\n\nDrawing commands via TCP are live."));

    QHBoxLayout* btnLayout = new QHBoxLayout();

    QPushButton* stopBtn = new QPushButton("Stop Bridge", this);
    connect(stopBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(stopBtn);

    QPushButton* hideBtn = new QPushButton("Hide", this);
    connect(hideBtn, &QPushButton::clicked, this, &QDialog::hide);
    btnLayout->addWidget(hideBtn);

    layout->addLayout(btnLayout);

    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    connect(m_timer, &QTimer::timeout, this, &MCP_Bridge_Dialog::processQueue);
    m_timer->start();

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &MCP_Bridge_Dialog::onNewConnection);
    if (!m_server->listen(QHostAddress::Any, 12346)) {
        LOG_STEP("ERROR: could not listen on port 12346");
    }
}

MCP_Bridge_Dialog::~MCP_Bridge_Dialog() {
    if (m_timer) m_timer->stop();
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
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (doc.isNull() || !doc.isObject()) {
        QJsonObject err;
        err["status"] = "error";
        err["message"] = QString("Invalid JSON: ") + parseErr.errorString();
        socket->write(QJsonDocument(err).toJson());
        socket->waitForBytesWritten(1000);
        return;
    }

    PendingCommand cmd;
    cmd.request = doc.object();
    cmd.socket = socket;
    m_queue.enqueue(cmd);
}

void MCP_Bridge_Dialog::processQueue() {
    if (m_queue.isEmpty()) return;

    PendingCommand cmd = m_queue.dequeue();
    executeCommand(cmd);
}

void MCP_Bridge_Dialog::executeCommand(const PendingCommand& cmd) {
    QJsonObject response;
    QString method = cmd.request["method"].toString();

    if (!m_doc) {
        response["status"] = "error";
        response["message"] = "No document available";
    } else {
        mcp::LibreCadDrawingAdapter adapter(m_doc, this);
        mcp::CommandProcessor processor(adapter);
        response = processor.process(cmd.request);
    }

    QByteArray respData = QJsonDocument(response).toJson(QJsonDocument::Compact);
    fprintf(stderr, "[MCP BRIDGE] %s -> %s\n", method.toUtf8().constData(), respData.constData());
    fflush(stderr);

    if (cmd.socket && cmd.socket->state() == QAbstractSocket::ConnectedState) {
        cmd.socket->write(respData);
        cmd.socket->write("\n");
        cmd.socket->waitForBytesWritten(1000);
    }
}
