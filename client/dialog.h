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
#include <QListWidget>
#include <QListWidgetItem>
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
    void chooseUser();

private slots:
    void on_pushButton_clicked();
    // void slotOnConnected();
    void slotDisconnected();
    void slotTextMessageReceived(const QString &message);
    void onUserSelected(QListWidgetItem *item);



private:
    Ui::Dialog *ui;
    QString login;
    QString password;
    QString toLogin;
    QWebSocket *socket;
    QJsonArray history;
    QHash<QString, QListWidgetItem*> userItemMap;
    // QByteArray Data;
    void SendToServer(QString str, QString toLogin);
    // quint16 nextBlockSize;
    void handleClients(const QJsonArray &clients);
    void handleAddNewClient(const QJsonObject &newClient);
    void handleRemoveClient(const QJsonObject &client);
    void getHistoryOfMessages();
    void loadChatHistory(const QString &user);

    void showInitialState();
    void restoreChatState();

};

#endif // DIALOG_H

