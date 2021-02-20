//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef APP_UI_MAINCHATWIDGET_H
#define APP_UI_MAINCHATWIDGET_H

#include "header.h"
class ChatWidget;
namespace Ui {
class MainChatWidget;
}

class MainChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainChatWidget(QWidget *parent = nullptr);
    ~MainChatWidget();

    void Initialize();
    virtual void timerEvent(QTimerEvent *event);

    //void Reset();
    void SetUserId(const QString& arrUserId_);
    void SetLoginTime(const TimeStamp& nTimeStamp_);
    void UpdateFriendList();
    void OffLine();
    void NeedToProcessUserList();

    void NewMessageCallback(const QByteArray& nByteSender_);
private:
    void UpdateNeedToProcessUserShow(const NDataStruct::DynArray<QByteArray>& arrUsers_);
    void NeedToProcessUserListCallback(const NDataStruct::DynArray<QByteArray>& arrUserIds_);
    void UpdateFriendCallback(const NDataStruct::DynArray<QByteArray>& arrUserIds_);
    void ErrorCallback(const TcpConnectionPtr&);


    void GetNeedToProcessMessagesCallback(const NDataStruct::DynArray<Message>& arrMessages_);
    //void ErrorCallback(const TcpConnectionPtr&);


private slots:
    void AddClicked(bool checked_);
    void ListDoubleClicked(const QModelIndex& index);
private:
    Ui::MainChatWidget *ui;


    mutable MutexLock m_nMutex;
    Condition m_nCondition;
    bool m_bError;
    bool m_bFriendValid;
    NDataStruct::DynArray<QByteArray> m_arrFriends;


    mutable MutexLock m_nMutexForGetNeedToProcessMessage;
    Condition m_nConditionForGetNeedToProcessMessage;
    int m_nGetNeedToProcessMessagesRet;

    mutable MutexLock m_nMutexForNeedToProcessUsers;
    Condition m_nConditionForNeedToProcessUsers;
    bool m_bNeedToProcessUserValid;
    NDataStruct::DynArray<QByteArray> m_arrNeedToProcessUsers;

    mutable MutexLock m_nMutexLogin;
    QString m_arrUserId;
    TimeStamp m_nLoginTime;

private:
    QStandardItemModel* m_pModel;
    QStandardItemModel* m_pActiveModel;

private:
    // chat-window manager
    NDataStruct::DynArray<std::pair<QByteArray, QWidget*>> m_arrMapWidget;
    QByteArray m_nChattingUserId;


    int m_nRefreshTimerId;
    ChatWidget* m_pChattingWidget;
};

#endif // MAINCHATWIDGET_H
