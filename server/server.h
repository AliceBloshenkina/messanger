#ifndef SERVER_H
#define SERVER_H

// #include <QTcpServer>
// #include <QTcpSocket>
#include <QDataStream>
#include <QVector>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlTableModel>
#include <QSqlError>
#include <QThreadPool>
#include <QRunnable>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

enum SystemMessage : quint16 {
    ChatMessage = 0,
    Login = 1,
    Registration = 2,
    Fail = 3,
    Success = 4,
    Connections = 5
};

class Server : public QObject
{
    Q_OBJECT

public:
    // Server();
    explicit Server(QObject *parent = nullptr);
    ~Server();


    // protected:
    // void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void slotNewConnection();
    void slotDisconnected();
    void slotTextMessageReceived(const QString &message);

private:
    QWebSocketServer *webSocketServer;
    // QMap<QTcpSocket*, QString> clients;
    QMap<QWebSocket*, QString> clients;
    QSqlDatabase db;
    // QSqlTableModel *model;
    // qintptr socketDescriptor;

    bool processLoginRequest(QWebSocket *socket, const QString &login, const QString &password);
    void sendMessageToClients(const QJsonObject &jsonIncoming, QWebSocket *socket = nullptr, bool status = false);
    bool registrateNewClients(QWebSocket *socket, const QString &login, const QString &password);
    bool executeQuery(const QString &queryString, const QMap<QString, QVariant> &params, QSqlQuery *query);
    QString generateSalt();
    QString hashPassword(const QString &password, const QString &salt);
    bool initializeDatabase();


    void handleLogin(QWebSocket* socket, const QJsonObject &jsonObj);
    void handleRegistration(QWebSocket* socket,const QJsonObject &jsonObj);
    void handleChatMessage(QWebSocket *socket, const QJsonObject &jsonObj);
    QJsonArray getOnlineClientsList(QWebSocket *socket);
    // QJsonArray get
    void notifyAllClients(const QString &newClientLogin, QWebSocket *socket, const QString &status);
    void addMessageToDatabase(const QJsonObject &jsonObj);
    void markMessagesAsRead(const QString &fromLogin, const QString &toLogin);
    QJsonArray getMessagesFromDatabase(const QString &login);
    QJsonArray getAllClients(const QString &login);
    QJsonArray getClientsByName(const QString &login, const QString &letters);
    QString checkOnlineStatus(const QString &login);
};

#endif // SERVER_H
