#include "dialog.h"
#include "ui_dialog.h"
#include "systemmessage.h"


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , socket(new QWebSocket())
    // , history(new QJsonArray())
{
    ui->setupUi(this);

    // connect(socket, &QWebSocket::connected, this, &Dialog::slotOnConnected);
    connect(socket, &QWebSocket::disconnected, this, &Dialog::slotDisconnected);
    connect(socket, &QWebSocket::textMessageReceived, this, &Dialog::slotTextMessageReceived);
    connect(ui->lineEdit_3, &QLineEdit::textEdited, this, &Dialog::onSearchUsers_textEdited);

}

Dialog::~Dialog()
{
    socket->close();
    delete ui;
    qDebug() << "Dialog destroyed.";
}


bool Dialog::socketConnect(SystemMessage typeMessage)
{
    if (socket->state() == QAbstractSocket::ConnectedState)
    {
        qDebug() << "Already connected!";
        return false;
    }

    // QSslConfiguration sslConfig;
    // sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    // socket->setSslConfiguration(sslConfig);
    socket->open(QUrl("ws://127.0.0.1:1111"));

    QJsonObject request;
    request["type"] = (typeMessage == SystemMessage::Login) ? "login" : "registration";
    request["login"] = login;
    request["password"] = password;

    connect(socket, &QWebSocket::connected, this, [=]() {
        QJsonDocument doc(request);
        socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
        qDebug() << "JSON request sent to server:" << doc.toJson(QJsonDocument::Indented);
    });

    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred), this, [](QAbstractSocket::SocketError error) {
        qDebug() << "Connection error:" << error;
    });

    return true;


}


void Dialog::SendToServer(QString str, QString toLogin)
{
    ui->textBrowser->append(login + ": " + str);
    ui->lineEdit->clear();
    QJsonObject request;
    request["type"] = "chat";
    request["from"] = login;
    request["to"] = toLogin;
    request["message"] = str;

    QJsonDocument doc(request);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

}



void Dialog::handleClients(const QJsonArray &clients)
{
    ui->userListWidget->clear();

    // for (const QJsonValue &clientValue : clients) {
    //     QJsonObject clientObj = clientValue.toObject();
    //     handleAddNewClient(clientObj);
    // }

    ui->textBrowser->clear();

    for (const QJsonValue &chatValue : history) {
        QJsonObject chatObj = chatValue.toObject();
        // QString otherUser = chatObj["otherUser"].toString();
        QJsonObject person;
        person["login"] = chatObj["otherUser"].toString();

        person["online"] = chatObj["online"].toString();

        qDebug() << " LOGIN ONLINE - " << person["login"] <<  person["online"];
        handleAddNewClient(person);
    }

    connect(ui->userListWidget, &QListWidget::itemClicked, this, &Dialog::onUserSelected);

    qDebug() << "User list updated with" << clients.size() << "clients.";
}

void Dialog::handleRemoveClient(const QJsonObject &client)
{
    if (!client.isEmpty()) {
        QString login = client["login"].toString();

        if (userItemMap.contains(login)) {

            QListWidgetItem *item = userItemMap[login];

            QString updatedText = login + " (offline)";
            item->setText(updatedText);
            item->setForeground(Qt::red);

            qDebug() << "Updated client to offline:" << login;
        } else {
            qDebug() << "Client not found:" << login;
        }
    }
}


void Dialog::handleAddNewClient(const QJsonObject &newClient)
{
    qDebug() << "Добавляем нового клиента в список";
    if (!newClient.isEmpty()) {
        qDebug() << "Новый клиент не пустой";
        QString login = newClient["login"].toString();
        QString onlineStatus = newClient["online"].toString();
qDebug() << "NEW LOGIN - " << login << onlineStatus;
        if (!login.isEmpty()) {

            qDebug() << "NEW LOGIN - " << login << onlineStatus;

            if (userItemMap.contains(login)) {

                qDebug() << "IMHERE ";
                QListWidgetItem *item = userItemMap[login];
                QString updatedText = login + " (" + (onlineStatus == "TRUE" ? "online" : "offline") + ")";
                item->setText(updatedText);


                 item->setForeground(onlineStatus == "TRUE" ? Qt::green : Qt::red);
            } else {

                qDebug() << "Добавляем 2";
                QString displayText = login + " (" + (onlineStatus == "TRUE" ? "online" : "offline") + ")";
                QListWidgetItem *item = new QListWidgetItem(login, ui->userListWidget);

                item->setText(displayText);
                item->setForeground(onlineStatus == "TRUE" ? Qt::green : Qt::red);

                userItemMap.insert(login, item);
                ui->userListWidget->addItem(item);
            }
        }
    }
}



void Dialog::set_login(QString login, QString password)
{
    this->login = login;
    this->password = password;
}

void Dialog::slotTextMessageReceived(const QString &message)
{
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) {
        qDebug() << "Faile to identify socket";
        return;
    }

    QJsonDocument docJson = QJsonDocument::fromJson(message.toUtf8());
    if(!docJson.isObject()){
        qDebug() << "Invalod format message";
        return;
    }

    QJsonObject jsonObj = docJson.object();
    QString typeMessage = jsonObj["type"].toString();

    if (typeMessage == "login") {

        if(jsonObj["status"] == "success"){
            ui->textBrowser->append(login + " successfully logged in.");


            if (jsonObj.contains("history_messages") ) { // && jsonObj["history_messages"].isArray()
                // qDebug() << "я начал историю";
                history = jsonObj["history_messages"].toArray();
                // handleClients(jsonObj["clients"].toArray());
                handleClients(jsonObj["history_messages"].toArray());
            } else {
                qDebug() << "Key 'history_messages' not found or is not an array.";
            }
            // handleClients(jsonObj["clients"].toArray());
            showInitialState();

            emit onSuccess();
        } else if (jsonObj["status"] == "fail") {
            ui->textBrowser->append(login + " login failed.");
            emit onError();
        }
    } else if (typeMessage == "registration"){
        if(jsonObj["status"] == "success"){
            ui->textBrowser->append(login + " successfully registered.");
            // убрать вывод всех клиентов на экран
            // handleClients(jsonObj["clients"].toArray());
            showInitialState();

            emit onSuccess();
        } else if (jsonObj["status"] == "fail") {
            ui->textBrowser->append(login + " registration failed.");
            emit onError();
        }
    } else if (typeMessage == "chat") {
        if(jsonObj["status"] == "success"){
            ui->textBrowser->append(jsonObj["from"].toString() + ": " + jsonObj["message"].toString());
        } else if (jsonObj["status"] == "fail") {
            ui->textBrowser->append("Message delivery failed: " + jsonObj["message"].toString());
        }
    } else if(typeMessage == "update_clients") {
        // ДОБАВИТЬ ЗАМЕНУ ТОЛЬКО ДЛЯ ЗАМЕНЫ КЛИЕНТОВ КОТОРЫЕ У НАС ЕСТЬ

        // if(jsonObj["online"] == "TRUE"){
        //     handleAddNewClient(jsonObj);
        // } else if (jsonObj["online"] == "FALSE"){
        //     handleRemoveClient(jsonObj);
        // }
    } else if (typeMessage == "search_users"){
        qDebug() << "Получил пользователей для поиска";
        onSearchUsers_dropdownAppend(jsonObj);

        // for (const QJsonValue &clientValue : clientsArray) {
        //     QJsonObject clientObj = clientValue.toObject();
        //     QString login = clientObj["login"].toString();
        //     qDebug() << "Логин пользователя:" << login;
        // }
        // ВЫЗОВ МЕТОДА ДЛЯ ОТОБРАЖЕНИЯ НА ЭКРАН

    } else if (typeMessage == "get_online_status"){
         qDebug() << "Получил назад статус";
        QJsonObject person;
        person["login"] = jsonObj["message"].toString();

        person["online"] = jsonObj["online"].toString();
        handleAddNewClient(person);
        QListWidgetItem *item = userItemMap[login];
        onUserSelected(item);
        // // QString selectedUser = userItemMap.key(item);

        // qDebug() << "Selected user:" << selectedUser;
        // restoreChatState();
        // ui->titleLabel->setText(person["login"].toString());
    } else {
        qDebug() << "Unknown message type.";
    }

}




void Dialog::onUserSelected(QListWidgetItem *item)
{
    if (!item) {
        qDebug() << "No item selected.";
        return;
    }
    QString selectedUser = userItemMap.key(item);

    qDebug() << "Selected user:" << selectedUser;
    restoreChatState();
    ui->titleLabel->setText(selectedUser);
    loadChatHistory(selectedUser);
    markMessagesAsRead(selectedUser);

    //ВЕРНУТЬ


}

void Dialog::onSearchUsers_textEdited()
{
    qDebug() << "Start find users";
    QString searchText = ui->lineEdit_3->text().toLower();

    qDebug() << "Name found - " << searchText;

    QJsonObject request;
    request["type"] = "search_users";
    request["from"] = login;
    // request["to"] = toLogin;
    request["message"] = searchText;

    QJsonDocument doc(request);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    // for (int i = 0; i < ui->userListWidget->count(); ++i) {
    //     QListWidgetItem *item = ui->userListWidget->item(i);
    //     item->setHidden(!item->text().toLower().contains(searchText));
    // }
}

// void Dialog::focusInEvent(QFocusEvent *event)
// {
//     if (ui->lineEdit_3->hasFocus()) {
//         qDebug() << "lineEdit_3 в фокусе!";

//     }
//     QDialog::focusInEvent(event);
// }


void Dialog::loadChatHistory(const QString &user)
{
    ui->textBrowser->clear();

    for (const QJsonValue &chatValue : history) {
        QJsonObject chatObj = chatValue.toObject();
        QString otherUser = chatObj["otherUser"].toString();

        // qDebug() << "Checking chat for user:" << userName << "against otherUser:" << otherUser;

        if (otherUser == user) {
            QJsonArray messages = chatObj["messages"].toArray();
            for (const QJsonValue &messageValue : messages) {
                QJsonObject messageObj = messageValue.toObject();
                QString sender = messageObj["sender"].toString();
                QString message = messageObj["message"].toString();
                QString timestamp = messageObj["timestamp"].toString();
                bool isRead = messageObj["is_read"].toBool();

                QString formattedMessage = "[" + timestamp + "] " + sender + ": " + message;
                if (!isRead) {
                    formattedMessage += " (непрочитано)";
                }
                ui->textBrowser->append(formattedMessage);
            }
            break;
        }
    }
}

void Dialog::onSearchUsers_dropdownAppend(const QJsonObject &jsonObj)
{
    QJsonArray users = jsonObj["clients"].toArray();

    if (users.isEmpty() || ui->lineEdit_3->text().isEmpty()) {
        if (userDropdown) userDropdown->hide();
        return;
    }

    if (!userDropdown) {
        userDropdown = new QListWidget(this);
        userDropdown->setWindowFlags(Qt::ToolTip);
        userDropdown->setAttribute(Qt::WA_DeleteOnClose);
        userDropdown->setFocusPolicy(Qt::NoFocus);

        // connect(userDropdown, &QListWidget::itemClicked, this, &Dialog::onUserSelected);

         qDebug() << "Userdropdown не упал";
        connect(userDropdown, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {

            ui->lineEdit_3->setText(item->text());
            // ui->lineEdit_3->setFocus();

            qDebug() << "Userdropdown не упал2";


            qDebug() << "Userdropdown не упал3";
            QJsonObject request;
            request["from"] = login;
            qDebug() <<"text - " << ui->lineEdit_3->text();
            request["message"] = item->text();

            request["type"] = "get_online_status";
            qDebug() << "Дай статус";
            QJsonDocument doc(request);
            socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
            qDebug() << "Userdropdown не упал4";

            userDropdown->hide();
            // onUserSelected(item);
            userDropdown->clear();
            ui->lineEdit_3->clear();

        });
    } else {
        userDropdown->clear();
    }

    for (const QJsonValue &userValue : users) {
        QString username = userValue.toObject()["login"].toString();
        userDropdown->addItem(username);
    }

    QPoint pos = ui->lineEdit_3->mapToGlobal(QPoint(0, ui->lineEdit_3->height()));
    userDropdown->move(pos);
    userDropdown->resize(ui->lineEdit_3->width(), 100);
    userDropdown->show();

    QTimer::singleShot(0, ui->lineEdit_3, SLOT(setFocus()));
}

void Dialog::markMessagesAsRead(const QString &client)
{
    QJsonObject request;
    request["type"] = "mark_as_read";
    request["from"] = login;
    request["to"] = client;

    QJsonDocument doc(request);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    // добавить обновление на экран
    qDebug() << "Sent mark_as_read request for chat with:" << client;
}

void Dialog::slotDisconnected()
{
    qDebug() << "Disconnected from server.";
    ui->textBrowser->append("Connection to server lost.");

    emit onError();

    QTimer::singleShot(5000, this, [this]() {
        qDebug() << "Attempting to reconnect...";
        socket->open(QUrl("ws://127.0.0.1:1111"));
    });
}


void Dialog::on_pushButton_clicked()
{
    SendToServer(ui->lineEdit->text(), ui->titleLabel->text());
}

void Dialog::showInitialState()
{
    ui->lineEdit->hide();
    ui->pushButton->hide();

    ui->textBrowser->setGeometry(140, 60, 350, 410);

    ui->textBrowser->clear();
    ui->textBrowser->setAlignment(Qt::AlignCenter);
    ui->textBrowser->append("Добро пожаловать! Выберите пользователя для начала чата.");
}

void Dialog::restoreChatState()
{
    ui->lineEdit->show();
    ui->pushButton->show();

    ui->textBrowser->clear();
    ui->textBrowser->setGeometry(140, 60, 350, 351);

    ui->textBrowser->setAlignment(Qt::AlignLeft);

}


//проверить повторную регистрацию
// почему не дает с нового акка зайти
//нажатие на кнопки в поиске
