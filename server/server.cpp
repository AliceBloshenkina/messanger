#include "server.h"

Server::Server(QObject *parent)
    : QObject(parent),
    webSocketServer(new QWebSocketServer(QStringLiteral("Chat Server"), QWebSocketServer::NonSecureMode, this))
{
    if (!db.isValid()) {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("./users_messanger.db");
        if (!db.open()) {
            qDebug() << "Failed to open database" << db.lastError().text();
            return;
        }

        // query = new QSqlQuery(db);
        QSqlQuery query(db);
        if (!query.exec("CREATE TABLE IF NOT EXISTS Users(Login TEXT UNIQUE, Password TEXT, Salt TEXT);")) {
            qDebug() << "Failed to create table" << query.lastError().text();
            return;
        }

    }

    if (webSocketServer->listen(QHostAddress::Any, 1111)){
        qDebug() << "Server started";
        connect(webSocketServer, &QWebSocketServer::newConnection, this, &Server::slotNewConnection);

    } else {
        qDebug() << "Failed to start server" << webSocketServer->errorString();
    }

    // ВЫВОД ВСЕХ ДАННЫХ
    // QSqlQuery query(db);
    // if (query.exec("SELECT * FROM Users")) {
    //     while (query.next()) {
    //         QString login = query.value("Login").toString();
    //         QString password = query.value("Password").toString();
    //         QString salt = query.value("Salt").toString();
    //         qDebug() << "Login:" << login << ", Password:" << password << ", Salt:" << salt;
    //     }
    //      qDebug() << "Pusto";
    // } else {
    //     qDebug() << "Error executing query:" << query.lastError().text();
    // }

    // удаление всех данных
    // QSqlQuery query(db);
    // if (query.exec("DELETE FROM Users")) {  // Убедитесь, что название таблицы правильное
    //     qDebug() << "All records deleted from Users_Messanger.";
    // } else {
    //     qDebug() << "Error deleting records:" << query.lastError().text();
    // }



}

void Server::slotNewConnection()
{

    QWebSocket *socket = webSocketServer->nextPendingConnection();
    if(!socket){
        qDebug() << "Failed to accepting incoming connection";
        return;
    }

    connect(socket, &QWebSocket::textMessageReceived, this, &Server::slotTextMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &Server::slotDisconnected);

    qDebug() << "New WebSocket connection from:" << socket->peerAddress().toString() << "Port:" << socket->peerPort();

}

void Server::slotTextMessageReceived(const QString &message)
{
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket){
        qDebug() << "Faile to identify socket";
        return;
    }

    qDebug() << "Message received from:" << socket->peerAddress().toString() << ":" << message;

    QJsonDocument docJson = QJsonDocument::fromJson(message.toUtf8());
    if(!docJson.isObject()){
        qDebug() << "Invalod format message";
        return;
    }

    QJsonObject jsonObj = docJson.object();
    QString typeMessage = jsonObj["type"].toString();

    if (typeMessage == "login") {
        handleLogin(socket, jsonObj);
    } else if (typeMessage == "registration"){
        handleRegistration(socket, jsonObj);
    } else if (typeMessage == "chat") {
        handleChatMessage(socket, jsonObj);
    } else {
        qDebug() << "Unknown message type.";
    }

}

void Server::handleLogin(QWebSocket* socket, const QJsonObject &jsonObj)
{
    if (!socket || jsonObj["login"].toString().isEmpty() || jsonObj["password"].toString().isEmpty()) {
        qDebug() << "Invalid login attempt: empty login or password.";
        // sendLoginResponse(socket, false, "Invalid credentials");
        return;
    }

    bool statusLogin = processLoginRequest(socket, jsonObj["login"].toString(), jsonObj["password"].toString());
    sendMessageToClients(jsonObj, socket, statusLogin);
    if(statusLogin)
        notifyAllClients(jsonObj["login"].toString(), socket);

}



void Server::handleRegistration(QWebSocket* socket, const QJsonObject &jsonObj)
{
    if (!socket || jsonObj["login"].toString().isEmpty() || jsonObj["password"].toString().isEmpty()) {
        qDebug() << "Invalid registration attempt: empty login or password.";
        // sendLoginResponse(socket, false, "Invalid credentials");
        // добавить тут и во входе обработку ошибок
        return;
    }

    bool statusRegistartion = registrateNewClients(socket, jsonObj["login"].toString(), jsonObj["password"].toString());
    sendMessageToClients(jsonObj, socket, statusRegistartion);
    if(statusRegistartion)
        notifyAllClients(jsonObj["login"].toString(), socket);
}

void Server::handleChatMessage(QWebSocket *socket, const QJsonObject &jsonObj)
{
    sendMessageToClients(jsonObj, socket);
}

void Server::slotDisconnected()
{
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket)
        return;

    if (clients.contains(socket)) {
        qDebug() << "Client disconnected:" << clients.value(socket);
        clients.remove(socket);
    } else {
        qDebug() << "Unknown client disconnected.";
    }

    socket->deleteLater();
}

bool Server::executeQuery(const QString &queryString, const QMap<QString, QVariant> &params, QSqlQuery *query)
{

    if (!query) {
        qDebug() << "Query pointer is null";
        return false;
    }
    query->prepare(queryString);


    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        QString placeholder = ":" + it.key();
        query->bindValue(placeholder, it.value());
    }

    if (!query->exec()) {
        qDebug() << "Query execution error:" << query->lastError().text();
        return false;
    }
    return true;
}

bool Server::processLoginRequest(QWebSocket *socket, const QString &login, const QString &password)
{

    db.transaction();



    QSqlQuery query(db);
    QString checkQuery = "SELECT * FROM Users WHERE Login = :login";
    QMap<QString, QVariant> params;
    params["login"] = login;

    if (!executeQuery(checkQuery, params, &query)) {
        qDebug() << "Error checking user in database. Rolling back transaction.";
        db.rollback();  // Откат транзакции
        // socket->disconnectFromHost();
        return false;
    }


    if (query.next()) {
        QString dbHash = query.value("Password").toString();
        QString dbSalt = query.value("Salt").toString();

        // Сравнение пароля
        if (hashPassword(password, dbSalt) == dbHash) {
            qDebug() << "User password is OK";

            // Добавление пользователя в список клиентов
            clients.insert(socket, login);

            // Коммит транзакции
            if (!db.commit()) {
                qDebug() << "Error committing transaction:" << db.lastError().text();
                // socket->disconnectFromHost();
                return false;
            }

            return true;
        } else {
            qDebug() << "User password is NOT OK";
            db.rollback(); // Откат транзакции
            // socket->disconnectFromHost();
            return false;
        }
    } else {
        qDebug() << "User not found";
        db.rollback(); // Откат транзакции
        // socket->disconnectFromHost();
        return false;
    }
}

QString Server::generateSalt()
{
    QByteArray salt = QByteArray::number(QRandomGenerator::global()->generate64(), 16);
    return QString(salt);
}

QString Server::hashPassword(const QString &password, const QString &salt)
{
    QByteArray saltedPassword = password.toUtf8() + salt.toUtf8();
    QByteArray hash = QCryptographicHash::hash(saltedPassword, QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

bool Server::registrateNewClients(QWebSocket *socket, const QString &login, const QString &password)
{

    db.transaction();  // Начало транзакции

    QString salt = generateSalt();
    QString hash = hashPassword(password, salt);
    QSqlQuery query(db);
    QString insertQuery = "INSERT INTO Users (Login, Password, Salt) VALUES (:login, :password, :salt)";
    QMap<QString, QVariant> params = { {"login", login}, {"password", hash}, {"salt", salt} };

    if (!executeQuery(insertQuery, params, &query)) {
        qDebug() << "Error adding user to database. Rolling back transaction.";
        db.rollback();  // Откат транзакции
        // socket->disconnectFromHost();
        return false;
    }

    if (!db.commit()) {  // Завершение транзакции
        qDebug() << "Error registration transaction:" << db.lastError().text();
        // socket->disconnectFromHost();
        return false;
    }

    clients.insert(socket, login);

    qDebug() << "User registered successfully";
    return true;
}

QJsonArray Server::getOnlineClientsList() {
    QJsonArray onlineClients;

    for (auto it = clients.begin(); it != clients.end(); ++it) {
        QJsonObject client;
        client["login"] = it.value(); // Логин клиента
        onlineClients.append(client);
    }

    return onlineClients;
}

void Server::sendMessageToClients(const QJsonObject &jsonIncoming, QWebSocket *socket, bool status)
{
    QString messageType = jsonIncoming["type"].toString();

    QJsonObject response;

    if (messageType == "login"){
        response["type"] = "login";
        response["to"] = jsonIncoming["login"];
        response["status"] = status ? "success" : "fail";
        response["message"] = status ? "Login successful" : "Invalid login or password";

        if(status){
            response["clients"] = getOnlineClientsList();
        }

    } else if (messageType == "registration") {
        response["type"] = "registration";
        response["to"] = jsonIncoming["login"];
        response["status"] = status ? "success" : "fail";
        response["message"] = status ? "Registration successful" : "Login is used, please try again";

        if(status){
            response["clients"] = getOnlineClientsList();
        }
        // добавить обработку других ошибок - мб статур error
    } else if (messageType == "chat") {
        response["type"] = "chat";
        response["from"] = jsonIncoming["from"];
        response["to"] = jsonIncoming["to"];
        response["message"] = jsonIncoming["message"];


        QWebSocket *tempSocket = clients.key(response["to"].toString(), nullptr);
        if (tempSocket){
            socket = tempSocket;
            response["status"] = "success";
        } else {
            response["status"] = "fail";
            response["message"] = "Client not found!";
        }
        // }else if (messageType == "newconnection") {

    } else {
        qDebug() << "Wrong type message";
    }

    QJsonDocument doc(response);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));


}

void Server::notifyAllClients(const QString &newClientLogin, QWebSocket *socket) {
    QJsonObject notification;
    notification["type"] = "new_client";
    notification["message"] = "A new client has connected";
    notification["login"] = newClientLogin;

    QJsonDocument doc(notification);
    QString message = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    for (QWebSocket *clientSocket : clients.keys()) {
        if (clientSocket && clientSocket != socket) {
            clientSocket->sendTextMessage(message);
        }
    }

    qDebug() << "Notification sent to all clients about new client:" << newClientLogin;
}


Server::~Server() {
    db.close();
}



