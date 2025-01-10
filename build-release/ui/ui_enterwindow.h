/********************************************************************************
** Form generated from reading UI file 'enterwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ENTERWINDOW_H
#define UI_ENTERWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EnterWindow
{
public:
    QWidget *centralwidget;
    QLabel *titleLabel;
    QLineEdit *lineEdit;
    QLineEdit *lineEdit_2;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QLabel *statusLabel;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *EnterWindow)
    {
        if (EnterWindow->objectName().isEmpty())
            EnterWindow->setObjectName("EnterWindow");
        EnterWindow->resize(400, 350);
        centralwidget = new QWidget(EnterWindow);
        centralwidget->setObjectName("centralwidget");
        titleLabel = new QLabel(centralwidget);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setGeometry(QRect(50, 20, 300, 50));
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont font;
        font.setPointSize(16);
        font.setBold(true);
        titleLabel->setFont(font);
        lineEdit = new QLineEdit(centralwidget);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setGeometry(QRect(100, 100, 200, 40));
        lineEdit_2 = new QLineEdit(centralwidget);
        lineEdit_2->setObjectName("lineEdit_2");
        lineEdit_2->setGeometry(QRect(100, 160, 200, 40));
        lineEdit_2->setEchoMode(QLineEdit::Password);
        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(100, 220, 90, 40));
        pushButton_2 = new QPushButton(centralwidget);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(210, 220, 90, 40));
        statusLabel = new QLabel(centralwidget);
        statusLabel->setObjectName("statusLabel");
        statusLabel->setGeometry(QRect(50, 280, 300, 30));
        statusLabel->setAlignment(Qt::AlignCenter);
        EnterWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(EnterWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 400, 26));
        EnterWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(EnterWindow);
        statusbar->setObjectName("statusbar");
        EnterWindow->setStatusBar(statusbar);

        retranslateUi(EnterWindow);

        QMetaObject::connectSlotsByName(EnterWindow);
    } // setupUi

    void retranslateUi(QMainWindow *EnterWindow)
    {
        EnterWindow->setWindowTitle(QCoreApplication::translate("EnterWindow", "Login", nullptr));
        centralwidget->setStyleSheet(QCoreApplication::translate("EnterWindow", "\n"
"     background-color: #f5f5f5;\n"
"     font-family: Arial, sans-serif;\n"
"    ", nullptr));
        titleLabel->setText(QCoreApplication::translate("EnterWindow", "Welcome Back", nullptr));
        titleLabel->setStyleSheet(QCoreApplication::translate("EnterWindow", "\n"
"      color: #333;\n"
"      padding: 10px;\n"
"      border-bottom: 2px solid #aaa;\n"
"    ", nullptr));
        lineEdit->setPlaceholderText(QCoreApplication::translate("EnterWindow", "Enter your login", nullptr));
        lineEdit->setStyleSheet(QCoreApplication::translate("EnterWindow", "\n"
"      border: 1px solid #aaa;\n"
"      border-radius: 5px;\n"
"      padding: 5px;\n"
"      background-color: #fff;\n"
"    ", nullptr));
        lineEdit_2->setPlaceholderText(QCoreApplication::translate("EnterWindow", "Enter your password", nullptr));
        lineEdit_2->setStyleSheet(QCoreApplication::translate("EnterWindow", "\n"
"      border: 1px solid #aaa;\n"
"      border-radius: 5px;\n"
"      padding: 5px;\n"
"      background-color: #fff;\n"
"    ", nullptr));
        pushButton->setText(QCoreApplication::translate("EnterWindow", "Log In", nullptr));
        pushButton->setStyleSheet(QCoreApplication::translate("EnterWindow", "\n"
"      background-color: #007BFF;\n"
"      color: #fff;\n"
"      border: none;\n"
"      border-radius: 5px;\n"
"      padding: 10px;\n"
"    ", nullptr));
        pushButton_2->setText(QCoreApplication::translate("EnterWindow", "Sign Up", nullptr));
        pushButton_2->setStyleSheet(QCoreApplication::translate("EnterWindow", "\n"
"      background-color: #28a745;\n"
"      color: #fff;\n"
"      border: none;\n"
"      border-radius: 5px;\n"
"      padding: 10px;\n"
"    ", nullptr));
        statusLabel->setText(QString());
        statusLabel->setStyleSheet(QCoreApplication::translate("EnterWindow", "\n"
"      color: red;\n"
"    ", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EnterWindow: public Ui_EnterWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ENTERWINDOW_H
