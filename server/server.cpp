#include "server.h"
#include <QFile>

Server::Server(QObject *parent)
    : QObject(parent),
    webSocketServer(new QWebSocketServer(QStringLiteral("Chat Server"), QWebSocketServer::NonSecureMode, this))
{
// Удаление базы данных
    // QFile::remove("./messanger_users.db");

    if (!db.isValid()) {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("./messanger_users.db");
        if (!db.open()) {
            qDebug() << "Failed to open database" << db.lastError().text();
            return;
        }


        if(!initializeDatabase()){
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

bool Server::initializeDatabase() {
    QSqlQuery query(db);

    if (!query.exec("CREATE TABLE IF NOT EXISTS Users ("
                    "Id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "Login TEXT UNIQUE NOT NULL, "
                    "Password TEXT NOT NULL, "
                    "Salt TEXT NOT NULL);")) {
        qDebug() << "Failed to create table Users:" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS Chats ("
                    "Id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "IdName1 INTEGER NOT NULL, "
                    "IdName2 INTEGER NOT NULL, "
                    "UNIQUE (IdName1, IdName2), "
                    "FOREIGN KEY (IdName1) REFERENCES Users(Id) ON DELETE CASCADE, "
                    "FOREIGN KEY (IdName2) REFERENCES Users(Id) ON DELETE CASCADE);")) {
        qDebug() << "Failed to create table Chats:" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS Messages ("
                    "Id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "ChatId INTEGER NOT NULL, "
                    "SenderId INTEGER NOT NULL, "
                    "Message TEXT NOT NULL, "
                    "Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                    "IsRead BOOLEAN DEFAULT 0, "
                    "FOREIGN KEY (ChatId) REFERENCES Chats(Id) ON DELETE CASCADE, "
                    "FOREIGN KEY (SenderId) REFERENCES Users(Id) ON DELETE CASCADE);")) {
        qDebug() << "Failed to create table Messages:" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_chat_messages ON Messages (ChatId, Timestamp);")) {
        qDebug() << "Failed to create index idx_chat_messages:" << query.lastError().text();
        return false;
    }

    qDebug() << "Database initialized successfully";
    return true;
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
    } else if (typeMessage == "search_users") {
        sendMessageToClients(jsonObj, socket);
    } else if ("get_online_status") {
        jsonObj["online"] = checkOnlineStatus(jsonObj["message"].toString());
        sendMessageToClients(jsonObj, socket);
        // qDebug() << jsonObj["online"];
    } else if ("mark_as_read"){
        markMessagesAsRead(jsonObj["from"].toString(), jsonObj["to"].toString());
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
        notifyAllClients(jsonObj["login"].toString(), socket, "TRUE");

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
        notifyAllClients(jsonObj["login"].toString(), socket, "TRUE");
}

void Server::handleChatMessage(QWebSocket *socket, const QJsonObject &jsonObj)
{
    QString toLogin = jsonObj["to"].toString();
    QWebSocket *recipientSocket = clients.key(toLogin, nullptr);

    addMessageToDatabase(jsonObj);

    if(recipientSocket){
        sendMessageToClients(jsonObj, socket);
    } else {
        qDebug() << "User " << toLogin << "is offline. Message saved in db.";
    }
}

void Server::addMessageToDatabase(const QJsonObject &jsonObj)
{
    if (!jsonObj.contains("from") || !jsonObj.contains("to") || !jsonObj.contains("message")) {
        qDebug() << "Invalid chat message format: missing fields.";
        return;
    }

    QString fromLogin = jsonObj["from"].toString();
    QString toLogin = jsonObj["to"].toString();
    QString message = jsonObj["message"].toString();

    if (fromLogin.isEmpty() || toLogin.isEmpty() || message.isEmpty()) {
        qDebug() << "Invalid chat message: empty sender, receiver, or message.";
        return;
    }

    QSqlQuery query(db);

    int fromId = -1, toId = -1;
    query.prepare("SELECT Id FROM Users WHERE Login = :login");
    query.bindValue(":login", fromLogin);
    if (query.exec() && query.next()) {
        fromId = query.value(0).toInt();
    } else {
        qDebug() << "Sender not found in database:" << fromLogin;
        return;
    }

    query.bindValue(":login", toLogin);
    if (query.exec() && query.next()) {
        toId = query.value(0).toInt();
    } else {
        qDebug() << "Receiver not found in database:" << toLogin;
        return;
    }

    int chatId = -1;
    query.prepare("SELECT Id FROM Chats WHERE (IdName1 = :fromId AND IdName2 = :toId) "
                  "OR (IdName1 = :toId AND IdName2 = :fromId)");
    query.bindValue(":fromId", fromId);
    query.bindValue(":toId", toId);

    if (query.exec() && query.next()) {
        chatId = query.value(0).toInt();
    } else {
        query.prepare("INSERT INTO Chats (IdName1, IdName2) VALUES (:fromId, :toId)");
        query.bindValue(":fromId", qMin(fromId, toId));
        query.bindValue(":toId", qMax(fromId, toId));
        if (query.exec()) {
            chatId = query.lastInsertId().toInt();
            qDebug() << "New chat created with ID:" << chatId;
        } else {
            qDebug() << "Failed to create chat:" << query.lastError().text();
            return;
        }
    }

    query.prepare("INSERT INTO Messages (ChatId, SenderId, Message, IsRead) VALUES (:chatId, :senderId, :message, 0)");
    query.bindValue(":chatId", chatId);
    query.bindValue(":senderId", fromId);
    query.bindValue(":message", message);

    if (!query.exec()) {
        qDebug() << "Failed to insert message into database:" << query.lastError().text();
        return;
    }

    qDebug() << "Message successfully added to database: chatId =" << chatId << ", senderId =" << fromId;
}

void Server::markMessagesAsRead(const QString &fromLogin, const QString &toLogin)
{
    QSqlQuery query(db);

    query.prepare("UPDATE Messages SET IsRead = 1 WHERE ChatId IN "
                  "(SELECT Id FROM Chats WHERE (IdName1 = (SELECT Id FROM Users WHERE Login = :from) "
                  "AND IdName2 = (SELECT Id FROM Users WHERE Login = :to)) "
                  "OR (IdName1 = (SELECT Id FROM Users WHERE Login = :to) "
                  "AND IdName2 = (SELECT Id FROM Users WHERE Login = :from))) "
                  "AND SenderId = (SELECT Id FROM Users WHERE Login = :to) "
                  "AND IsRead = 0");
    query.bindValue(":from", fromLogin);
    query.bindValue(":to", toLogin);

    if (!query.exec()) {
        qDebug() << "Failed to mark messages as read:" << query.lastError().text();
    } else {
        qDebug() << "Messages marked as read between" << fromLogin << "and" << toLogin;
    }
}


QJsonArray Server::getMessagesFromDatabase(const QString &login)
{
    QJsonArray chatsArray;

    int userId = -1;
    QSqlQuery query(db);
    query.prepare("SELECT Id FROM Users WHERE Login = :login");
    query.bindValue(":login", login);
    if (query.exec() && query.next()) {
        userId = query.value(0).toInt();
    } else {
        qDebug() << "User not found in database:" << login;
        return chatsArray;
    }

    query.prepare("SELECT Id, CASE WHEN IdName1 = :userId THEN IdName2 ELSE IdName1 END AS OtherUserId "
                  "FROM Chats WHERE IdName1 = :userId OR IdName2 = :userId");
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qDebug() << "Failed to retrieve chats for user:" << query.lastError().text();
        return chatsArray;
    }

    while (query.next()) {
        int chatId = query.value("Id").toInt();
        int otherUserId = query.value("OtherUserId").toInt();

        QSqlQuery userQuery(db);
        userQuery.prepare("SELECT Login FROM Users WHERE Id = :id");
        userQuery.bindValue(":id", otherUserId);
        QString otherUserName;
        if (userQuery.exec() && userQuery.next()) {
            otherUserName = userQuery.value(0).toString();
        } else {
            qDebug() << "Failed to retrieve username for userId:" << otherUserId;
            continue;
        }

        QSqlQuery messagesQuery(db);
        messagesQuery.prepare("SELECT SenderId, Message, Timestamp FROM Messages WHERE ChatId = :chatId ORDER BY Timestamp ASC");
        messagesQuery.bindValue(":chatId", chatId);

        QJsonArray messagesArray;
        if (messagesQuery.exec()) {
            while (messagesQuery.next()) {
                QJsonObject messageObj;
                int senderId = messagesQuery.value("SenderId").toInt();
                messageObj["sender"] = (senderId == userId) ? login : otherUserName;
                messageObj["message"] = messagesQuery.value("Message").toString();
                messageObj["timestamp"] = messagesQuery.value("Timestamp").toString();
                messageObj["is_read"] = messagesQuery.value("IsRead").toInt();
                messagesArray.append(messageObj);
            }
        } else {
            qDebug() << "Failed to retrieve messages for chatId:" << chatId;
            continue;
        }

        QJsonObject chatObj;
        chatObj["otherUser"] = otherUserName;
        chatObj["messages"] = messagesArray;
        // qDebug() << "ИМЯ " << otherUserName;
        if(clients.key(otherUserName, nullptr)){
            chatObj["online"] = "TRUE";
            // qDebug() << "статус тру";
        } else {
            chatObj["online"] = "FALSE";
            // qDebug() << "статус false";
        }
        chatsArray.append(chatObj);
    }

    return chatsArray;

}

QJsonArray Server::getAllClients(const QString &login)
{
    QJsonArray allClients;

    QSqlQuery query(db);
    query.prepare("SELECT Login FROM Users");

    if (!query.exec()) {
        qDebug() << "Some problem with get all clients" << query.lastError().text();
        return allClients;
    }

    while(query.next()){
        QString currentLogin = query.value("Login").toString();
        if(currentLogin != login){
            QJsonObject client;
            QString currentLogin = query.value("Login").toString();
            client["login"] = currentLogin;
            if(clients.key(currentLogin, nullptr)){
                client["online"] = "TRUE";
            } else {
                client["online"] = "FALSE";
            }
            allClients.append(client);
        }
    }

    return allClients;
}

QJsonArray Server::getClientsByName(const QString &login, const QString &letters)
{
    QJsonArray allClients;

    QSqlQuery query(db);
    query.prepare("SELECT Login FROM Users WHERE Login LIKE :letters");
    query.bindValue(":letters", letters + "%");


    if (!query.exec()) {
        qDebug() << "Some problem with get all clients" << query.lastError().text();
        return allClients;
    }

    while(query.next()){
        QString currentLogin = query.value("Login").toString();
        if(currentLogin != login){
            QJsonObject client;
            QString currentLogin = query.value("Login").toString();
            client["login"] = currentLogin;
            if(clients.key(currentLogin, nullptr)){
                client["online"] = "TRUE";
            } else {
                client["online"] = "FALSE";
            }
            allClients.append(client);
        }
    }

    return allClients;
}

QString Server::checkOnlineStatus(const QString &login)
{
    // bool isOnline = std::any_of(clients.begin(), clients.end(),
    //                             [&login](const auto &it) { return it.value() == login; });


    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it.value() == login) {
            return "TRUE";
        }
    }
    return "FALSE";

    // return isOnline ? "TRUE" : "FALSE";
}



void Server::slotDisconnected()
{
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket)
        return;

    if (clients.contains(socket)) {
        qDebug() << "Client disconnected:" << clients.value(socket);

        notifyAllClients(clients[socket], socket, "FALSE");
        // QJsonObject notification;
        // notification["type"] = "clients";
        // notification["status"] = "disconnect";
        // notification["message"] = "A new client has connected";
        // notification["login"] = clients[socket];

        // QJsonDocument doc(notification);
        // QString message = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

        // for (QWebSocket *clientSocket : clients.keys()) {
        //     if (clientSocket && clientSocket != socket) {
        //         clientSocket->sendTextMessage(message);
        //     }
        // }

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
        db.rollback();
        // socket->disconnectFromHost();
        return false;
    }


    if (query.next()) {
        QString dbHash = query.value("Password").toString();
        QString dbSalt = query.value("Salt").toString();

        if (hashPassword(password, dbSalt) == dbHash) {
            qDebug() << "User password is OK";

            clients.insert(socket, login);

            if (!db.commit()) {
                qDebug() << "Error committing transaction:" << db.lastError().text();
                // socket->disconnectFromHost();
                return false;
            }

            return true;
        } else {
            qDebug() << "User password is NOT OK";
            db.rollback();
            // socket->disconnectFromHost();
            return false;
        }
    } else {
        qDebug() << "User not found";
        db.rollback();
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

    if (login.isEmpty() || password.isEmpty()) {
        qDebug() << "Registration failed: login or password is empty.";
        return false;
    }

    db.transaction();

    QString salt = generateSalt();
    QString hash = hashPassword(password, salt);
    QSqlQuery query(db);
    QString insertQuery = "INSERT INTO Users (Login, Password, Salt) VALUES (:login, :password, :salt)";
    QMap<QString, QVariant> params = { {"login", login}, {"password", hash}, {"salt", salt} };

    if (!executeQuery(insertQuery, params, &query)) {
        qDebug() << "Error adding user to database. Rolling back transaction.";
        db.rollback();
        // socket->disconnectFromHost();
        return false;
    }

    if (!db.commit()) {
        qDebug() << "Error registration transaction:" << db.lastError().text();
        // socket->disconnectFromHost();
        return false;
    }

    clients.insert(socket, login);

    qDebug() << "User registered successfully";
    return true;
}

QJsonArray Server::getOnlineClientsList(QWebSocket *socket) {
    QJsonArray onlineClients;

    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if(socket != it.key()){
            QJsonObject client;
            client["login"] = it.value(); // Логин клиента
            onlineClients.append(client);
        }
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
            // response["clients"] = getAllClients(jsonIncoming["login"].toString());
            //getOnlineClientsList(socket);
            response["history_messages"] = getMessagesFromDatabase(jsonIncoming["login"].toString());

        }

    } else if (messageType == "registration") {
        response["type"] = "registration";
        response["to"] = jsonIncoming["login"];
        response["status"] = status ? "success" : "fail";
        response["message"] = status ? "Registration successful" : "Login is used, please try again";

        if(status){
            // response["clients"] = getAllClients(jsonIncoming["login"].toString());//getOnlineClientsList(socket);
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
            // addMessageToDatabase(jsonIncoming);
        } else {
            response["status"] = "fail";
            response["message"] = "Client not found!";
        }
    } else if (messageType == "search_users") {
        response["type"] = "search_users";
        response["to"] = jsonIncoming["login"];
        // response["status"] = status ? "success" : "fail";
        response["clients"] = getClientsByName(jsonIncoming["login"].toString(), jsonIncoming["message"].toString());
    } else if (messageType == "get_online_status"){
        qDebug() << "Отправил назад статут онлайн";
        response["type"] = "get_online_status";
        response["to"] = jsonIncoming["login"];
        response["online"] = jsonIncoming["online"];
        response["message"] = jsonIncoming["message"];
    } else {
        qDebug() << "Wrong type message";
    }

    QJsonDocument doc(response);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));


}

void Server::notifyAllClients(const QString &newClientLogin, QWebSocket *socket, const QString &status) {
    QJsonObject notification;
    notification["type"] = "update_clients";
    // notification["status"] = status; //"connect";
    // notification["message"] = "A new client has connected";
    notification["login"] = newClientLogin;
    notification["online"] = status;

    QJsonDocument doc(notification);
    QString message = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    for (QWebSocket *clientSocket : clients.keys()) {
        if (clientSocket && clientSocket != socket) {
            qDebug() << "Sending notification to client:" << clients[clientSocket];
            clientSocket->sendTextMessage(message);
        } else {
            qDebug() << "Skipping notification for socket:" << clients[clientSocket];
        }
    }

        qDebug() << "Notification about" << newClientLogin << "with status" << status << "sent to all clients.";
}


Server::~Server() {
    db.close();
}



