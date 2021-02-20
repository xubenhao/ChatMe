//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef APP_UI_ADDFRIENDDIALOG_H
#define APP_UI_ADDFRIENDDIALOG_H

#include "header.h"

namespace Ui {
class AddFriendDialog;
}

class AddFriendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFriendDialog(QWidget *parent = nullptr);
    ~AddFriendDialog();

    void Initialize();
    void SetMasterUserId(const QByteArray& arrUserId_){m_arrMasterUserId = arrUserId_;}

//private:
//    void UpdateFriendCallback(NDataStruct::DynArray<QByteArray> arrUserIds_);

private slots:
    void AddClicked(bool checked_);
    void ExitClicked(bool checked_);

private:
    void AddCallback(int nType_);
    void ErrorCallback(const TcpConnectionPtr&);

private:
    mutable MutexLock m_nMutex;
    Condition m_nCondition;
    int m_nAddRet;
    bool m_bError;

    QByteArray m_arrMasterUserId;
private:
    Ui::AddFriendDialog *ui;
};

#endif // ADDFRIENDDIALOG_H
