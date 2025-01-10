#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
// #include <QWebSocket>
// #include <QWebSocketServer>
// #include <QTcpSocket>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QTimer>
#include "systemmessage.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public slots:
              // void slotReadyRead();

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();
    void set_login(QString login, QString password);
    bool socketConnect(SystemMessage typeMessage);

signals:
    void onSuccess();
    void onError();
    void somethingWrong();

private slots:
    void on_pushButton_clicked();
    // void slotOnConnected();
    void slotDisconnected();
    void slotTextMessageReceived(const QString &message);



private:
    Ui::Dialog *ui;
    QString login;
    QString password;
    QString toLogin;
    QWebSocket *socket;
    // QByteArray Data;
    void SendToServer(QString str, QString toLogin);
    // quint16 nextBlockSize;
    void handleClients(QJsonArray clients);
    void handleAddNewClient(QJsonObject newClient);
    void handleRemoveNewClient(QJsonObject client);

    void showInitialState();
    void restoreChatState();

};

#endif // DIALOG_H

