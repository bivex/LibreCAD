#ifndef MCP_BRIDGE_H
#define MCP_BRIDGE_H

#include <QObject>
#include <QDialog>
#include <QTcpServer>
#include <QTimer>
#include <QQueue>
#include "qc_plugininterface.h"
#include "document_interface.h"

class MCP_Bridge_Dialog;

class MCP_Bridge : public QObject, public QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE "mcp_bridge.json")

public:
    MCP_Bridge();
    virtual ~MCP_Bridge();

    virtual QString name() const override;
    virtual PluginCapabilities getCapabilities() const override;
    virtual void execComm(Document_Interface* doc, QWidget* parent, QString cmd) override;

private:
    MCP_Bridge_Dialog* m_dialog = nullptr;
};

struct PendingCommand {
    QJsonObject request;
    QTcpSocket* socket;
};

class MCP_Bridge_Dialog : public QDialog {
    Q_OBJECT
public:
    MCP_Bridge_Dialog(Document_Interface* doc, QWidget* parent);
    ~MCP_Bridge_Dialog();

    void hideAndKeepRunning();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void processQueue();

private:
    void executeCommand(const PendingCommand& cmd);

    Document_Interface* m_doc;
    QTcpServer* m_server;
    QPushButton* m_stopBtn;
    QPushButton* m_hideBtn;
    QTimer* m_timer;
    QQueue<PendingCommand> m_queue;
};

#endif
