//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef APP_UI_MAINWINDOW_H
#define APP_UI_MAINWINDOW_H

#include "header.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void Initialize();
    virtual void closeEvent(QCloseEvent *event);
private slots:
    void ShowDataStruct(const QString& strName_);
    void ShowMainChatWidget(const QByteArray& strUserId_, const TimeStamp& nTimeStamp);

private:
    Ui::MainWindow *ui;
    NDataStruct::DynArray<std::pair<QByteArray, QWidget*>> m_arrMapWidget;
};

#endif // MAINWINDOW_H
