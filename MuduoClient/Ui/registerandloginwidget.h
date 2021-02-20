//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef APP_UI_REGISTERANDLOGINWIDGET_H
#define APP_UI_REGISTERANDLOGINWIDGET_H

#include "header.h"

namespace Ui {
class RegisterAndLoginWidget;
}

class RegisterAndLoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterAndLoginWidget(QWidget *parent = nullptr);
    ~RegisterAndLoginWidget();

    void Initialize();

private slots:
    void OnLoginClicked(bool bFlag_);
    void OnRegisterClicked(bool bFlag_);

private:
    void RegisterCallback(int nType_);
    void ErrorCallback(const TcpConnectionPtr&);
    void LoginCallback(int nType_, TimeStamp nTimeStamp_);


Q_SIGNALS:
    //void ShowDataStruct(const QString& strName_);
    void ShowMainChatWidget(const QByteArray& strUserId_, const TimeStamp& nTimeStamp_);
private:
    Ui::RegisterAndLoginWidget *ui;

    mutable MutexLock m_nMutex;
    Condition m_nCondition;
    int m_nRegisterRet;
    int m_nLoginRet;
    TimeStamp m_nTimeStamp;
    bool m_bError;


};

#endif // REGISTERANDLOGINWIDGET_H
