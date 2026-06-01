#ifndef MCP_BRIDGE_H
#define MCP_BRIDGE_H

#include <QObject>
#include <QTcpServer>
#include <QJsonObject>
#include "qc_plugininterface.h"

class MCP_Bridge : public QObject, public QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE "mcp_bridge.json")

public:
    MCP_Bridge();
    virtual ~MCP_Bridge();

    virtual QString name() const override { return "MCP Bridge"; }
    virtual PluginCapabilities getCapabilities() const override;
    virtual void execComm(Document_Interface* doc, QWidget* parent, QString cmd) override;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void handleCommand(const QJsonObject& json);
    
    QTcpServer* m_server;
    QWidget* m_parent;
    Document_Interface* m_doc;
};

#endif
