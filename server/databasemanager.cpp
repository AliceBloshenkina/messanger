#include "databasemanager.h"

DatabaseManager::DatabaseManager() {
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

DatabaseManager::~DatabaseManager() {
    db.close();
}

bool DatabaseManager::openDatabase() {
    if (!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::initializeDatabase() {
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
                    "Status TEXT DEFAULT 'sent', "
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

bool DatabaseManager::userExists(const QString &login) {
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM Users WHERE Login = :login");
    query.bindValue(":login", login);
    if (!query.exec() || !query.next()) {
        return false;
    }
    return query.value(0).toInt() > 0;
}

bool DatabaseManager::addUser(const QString &login, const QString &password, const QString &salt) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO Users (Login, Password, Salt) VALUES (:login, :password, :salt)");
    query.bindValue(":login", login);
    query.bindValue(":password", password);
    query.bindValue(":salt", salt);
    return query.exec();
}

bool DatabaseManager::checkUserPassword(const QString &login, const QString &password) {
    QSqlQuery query(db);
    query.prepare("SELECT Password, Salt FROM Users WHERE Login = :login");
    query.bindValue(":login", login);

    if (!query.exec() || !query.next()) {
        return false;
    }

    QString dbHash = query.value(0).toString();
    QString dbSalt = query.value(1).toString();

    QByteArray saltedPassword = password.toUtf8() + dbSalt.toUtf8();
    QByteArray hash = QCryptographicHash::hash(saltedPassword, QCryptographicHash::Sha256);

    return (dbHash == hash.toHex());
}

QJsonArray DatabaseManager::getMessages(const QString &login, const QMap<QWebSocket*, QString> &clients)
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
        qDebug() << "ИМЯ " << otherUserName;
        if(clients.key(otherUserName, nullptr)){
            chatObj["online"] = "TRUE";
        } else {
            chatObj["online"] = "FALSE";
        }
        chatsArray.append(chatObj);
    }

    return chatsArray;
}

void DatabaseManager::addMessage(const QString &from, const QString &to, const QString &message)
{
    if (from.isEmpty() || to.isEmpty() || message.isEmpty()) {
        qDebug() << "Invalid chat message: empty sender, receiver, or message.";
        return;
    }

    QSqlQuery query(db);

    int fromId = -1, toId = -1;
    query.prepare("SELECT Id FROM Users WHERE Login = :login");
    query.bindValue(":login", from);
    if (query.exec() && query.next()) {
        fromId = query.value(0).toInt();
    } else {
        qDebug() << "Sender not found in database:" << from;
        return;
    }

    query.bindValue(":login", to);
    if (query.exec() && query.next()) {
        toId = query.value(0).toInt();
    } else {
        qDebug() << "Receiver not found in database:" << to;
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

    query.prepare("INSERT INTO Messages (ChatId, SenderId, Message, Status) "
                  "VALUES (:chatId, :senderId, :message, 'sent')");
    query.bindValue(":chatId", chatId);
    query.bindValue(":senderId", fromId);
    query.bindValue(":message", message);

    if (!query.exec()) {
        qDebug() << "Failed to insert message into database:" << query.lastError().text();
        return;
    }

    qDebug() << "Message successfully added to database: chatId =" << chatId << ", senderId =" << fromId;

}

void DatabaseManager::markMessagesAsRead(const QString &from, const QString &to, const QString &msgId)
{
    QSqlQuery query(db);

    if(msgId.isEmpty()){
        query.prepare("UPDATE Messages SET IsRead = 1 WHERE ChatId IN "
                      "(SELECT Id FROM Chats WHERE (IdName1 = (SELECT Id FROM Users WHERE Login = :from) "
                      "AND IdName2 = (SELECT Id FROM Users WHERE Login = :to)) "
                      "OR (IdName1 = (SELECT Id FROM Users WHERE Login = :to) "
                      "AND IdName2 = (SELECT Id FROM Users WHERE Login = :from))) "
                      "AND SenderId = (SELECT Id FROM Users WHERE Login = :to) "
                      "AND IsRead = 0");
        query.bindValue(":from", from);
        query.bindValue(":to", to);
    } else {

        query.prepare("UPDATE Messages SET Status = 'read' WHERE Id = :msgId");
        query.bindValue(":msgId", msgId);
        // if (query.exec()) {
        //     qDebug() << "Message" << msgId << "marked as read.";
        // }
    }

    if (!query.exec()) {
        qDebug() << "Failed to mark messages as read:" << query.lastError().text();
    } else {
        qDebug() << "Messages marked as read between" << from << "and" << to;
    }
}

QJsonArray DatabaseManager::getUsersByName(const QMap<QWebSocket*, QString> &clients, const QString &login, const QString &letters)
{
    QJsonArray users;

    QSqlQuery query(db);
    query.prepare("SELECT Login FROM Users WHERE Login LIKE :letters");
    query.bindValue(":letters", letters + "%");


    if (!query.exec()) {
        qDebug() << "Some problem with get all clients" << query.lastError().text();
        return users;
    }

    while(query.next()){
        QString currentLogin = query.value("Login").toString();
        if(currentLogin != login){
            QJsonObject user;
            QString currentLogin = query.value("Login").toString();
            user["login"] = currentLogin;
            if(clients.key(currentLogin, nullptr)){
                user["online"] = "TRUE";
            } else {
                user["online"] = "FALSE";
            }
            users.append(user);
        }
    }

    return users;
}

bool DatabaseManager::executeQuery(const QString &queryString, const QMap<QString, QVariant> &params, QSqlQuery *query)
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

QString DatabaseManager::generateSalt()
{
    QByteArray salt = QByteArray::number(QRandomGenerator::global()->generate64(), 16);
    return QString(salt);
}

QString DatabaseManager::hashPassword(const QString &password, const QString &salt)
{
    QByteArray saltedPassword = password.toUtf8() + salt.toUtf8();
    QByteArray hash = QCryptographicHash::hash(saltedPassword, QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

bool DatabaseManager::registrateNewClients(const QString &login, const QString &password)
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
        return false;
    }

    if (!db.commit()) {
        qDebug() << "Error registration transaction:" << db.lastError().text();
        return false;
    }



    qDebug() << "User registered successfully";
    return true;
}
