#include "dialog.h"
#include "ui_dialog.h"
#include "systemmessage.h"


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , socket(new QWebSocket())
{
    ui->setupUi(this);

    // connect(socket, &QWebSocket::connected, this, &Dialog::slotOnConnected);
    connect(socket, &QWebSocket::disconnected, this, &Dialog::slotDisconnected);
    connect(socket, &QWebSocket::textMessageReceived, this, &Dialog::slotTextMessageReceived);


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
    // sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // Отключение проверки сертификата (только для разработки)
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
        // return false; //???????????????
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

void Dialog::handleClients(QJsonArray clients)
{
    // Очищаем список пользователей перед добавлением новых
    ui->userListWidget->clear();

    // Проходимся по каждому объекту в массиве
    for (const QJsonValue &clientValue : clients) {
        if (clientValue.isObject()) {
            QJsonObject clientObj = clientValue.toObject();
            QString login = clientObj["login"].toString(); // Получаем логин пользователя

            if (!login.isEmpty()) {
                // Создаем элемент списка
                QListWidgetItem *item = new QListWidgetItem(login, ui->userListWidget);
                ui->userListWidget->addItem(item);
            }
        }
    }

    // Подключаем обработчик нажатия на элемент списка
    connect(ui->userListWidget, &QListWidget::itemClicked, this, [=](QListWidgetItem *item) {
        if (item) {
            ui->titleLabel->setText(item->text()); // Устанавливаем выбранный логин в поле "To"
        }
    });

    qDebug() << "User list updated with" << clients.size() << "clients.";
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

    qDebug() << "Message received from:" << socket->peerAddress().toString() << ":" << message;

    QJsonDocument docJson = QJsonDocument::fromJson(message.toUtf8());
    if(!docJson.isObject()){
        qDebug() << "Invalod format message";
        return;
    }

    QJsonObject jsonObj = docJson.object();
    QString typeMessage = jsonObj["type"].toString();

    if (typeMessage == "login") {
        // handleLogin(socket, jsonObj);
        if(jsonObj["status"] == "success"){
            ui->textBrowser->append(login + " successfully logged in.");

            //вывод списка клиентов на экран
            handleClients(jsonObj["clients"].toArray());
            emit onSuccess();
        } else if (jsonObj["status"] == "fail") {
            ui->textBrowser->append(login + " login failed.");
            emit onError();
        }
    } else if (typeMessage == "registration"){
        // handleRegistration(socket, jsonObj);
        if(jsonObj["status"] == "success"){
            ui->textBrowser->append(login + " successfully registered.");
            handleClients(jsonObj["clients"].toArray());
            //вывод списка клиентов на экран
            emit onSuccess();
        } else if (jsonObj["status"] == "fail") {
            ui->textBrowser->append(login + " registration failed.");
            emit onError();
        }
    } else if (typeMessage == "chat") {
        // handleChatMessage(socket, jsonObj);
        if(jsonObj["status"] == "success"){
            ui->textBrowser->append(jsonObj["from"].toString() + ": " + jsonObj["message"].toString());
        } else if (jsonObj["status"] == "fail") {
            ui->textBrowser->append("Message delivery failed: " + jsonObj["message"].toString());
        }
    } else if(typeMessage == "new_client") {
        //внести добавление нового клиента к списку клиентов
    } else {
        qDebug() << "Unknown message type.";
    }




}

void Dialog::slotDisconnected()
{
    qDebug() << "Disconnected from server.";
    ui->textBrowser->append("Connection to server lost.");

    emit onError();

    // Попытка переподключения через 5 секунд
    QTimer::singleShot(5000, this, [this]() {
        qDebug() << "Attempting to reconnect...";
        socket->open(QUrl("ws://127.0.0.1:1111"));
    });
}


void Dialog::on_pushButton_clicked()
{
    SendToServer(ui->lineEdit->text(), ui->titleLabel->text());
}



//проверить повторную регистрацию
// почему не дает с нового акка зайти
//придумать список диалогов
