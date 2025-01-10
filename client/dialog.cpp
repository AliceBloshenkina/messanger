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
        QJsonObject clientObj = clientValue.toObject();
        handleAddNewClient(clientObj);
        // if (clientValue.isObject()) {
        //     QJsonObject clientObj = clientValue.toObject();
        //     QString login = clientObj["login"].toString(); // Получаем логин пользователя

        //     if (!login.isEmpty()) {
        //         // Создаем элемент списка
        //         QListWidgetItem *item = new QListWidgetItem(login, ui->userListWidget);
        //         ui->userListWidget->addItem(item);
        //     }
        // }
    }

    // Подключаем обработчик нажатия на элемент списка
    connect(ui->userListWidget, &QListWidget::itemClicked, this, [=](QListWidgetItem *item) {
        if (item) {
            restoreChatState();
            ui->titleLabel->setText(item->text()); // Устанавливаем выбранный логин в поле "To"
        }
    });

    qDebug() << "User list updated with" << clients.size() << "clients.";
}

void Dialog::handleRemoveNewClient(QJsonObject client)
{
    if (!client.isEmpty()) {
        QString login = client["login"].toString(); // Получаем логин пользователя

        if (!login.isEmpty()) {
            // Проходим по всем элементам QListWidget
            for (int i = 0; i < ui->userListWidget->count(); ++i) {
                QListWidgetItem *item = ui->userListWidget->item(i);
                if (item && item->text() == login) { // Сравниваем текст элемента с логином
                    delete ui->userListWidget->takeItem(i); // Удаляем элемент из списка
                    break; // Останавливаем цикл после удаления
                }
            }
        }
    }
}


void Dialog::handleAddNewClient(QJsonObject newClient)
{
    // for (int i = 0; i < ui->userListWidget->count(); ++i) {
    //     QListWidgetItem *item = ui->userListWidget->item(i);
    //     if (item->text() == login) {
    //         qDebug() << "Client already in the list:" << login;
    //         return; // Не добавляем дублирующий элемент
    //     }
    // }

    if (!newClient.isEmpty()) {
        // QJsonObject clientObj = clientValue.toObject();
        QString login = newClient["login"].toString(); // Получаем логин пользователя

        if (!login.isEmpty()) {
            // Создаем элемент списка
            QListWidgetItem *item = new QListWidgetItem(login, ui->userListWidget);
            ui->userListWidget->addItem(item);
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
            showInitialState();
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
            showInitialState();
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
    } else if(typeMessage == "clients") {
        if(jsonObj["status"] == "connect"){
            handleAddNewClient(jsonObj);
        } else if (jsonObj["status"] == "disconnect"){
            handleRemoveNewClient(jsonObj);
        }
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

void Dialog::showInitialState()
{
    // Скрываем lineEdit и pushButton
    ui->lineEdit->hide();
    ui->pushButton->hide();

    // Увеличиваем textBrowser
    ui->textBrowser->setGeometry(140, 60, 350, 410); // Растянуть на область lineEdit и pushButton

    // Отображаем текст в центре
    ui->textBrowser->clear();
    ui->textBrowser->setAlignment(Qt::AlignCenter);
    ui->textBrowser->append("Добро пожаловать! Выберите пользователя для начала чата.");
}

void Dialog::restoreChatState()
{
    // Показываем lineEdit и pushButton
    ui->lineEdit->show();
    ui->pushButton->show();

    ui->textBrowser->clear();
    // Восстанавливаем размер textBrowser
    ui->textBrowser->setGeometry(140, 60, 350, 351);

    // Очищаем выравнивание текста
    ui->textBrowser->setAlignment(Qt::AlignLeft);
}


//проверить повторную регистрацию
// почему не дает с нового акка зайти
//придумать список диалогов
