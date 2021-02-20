//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef APP_UI_CHATWIDGET_H
#define APP_UI_CHATWIDGET_H

#include "header.h"

namespace Ui {
class ChatWidget;
}


class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    void Initialize();
    void SetNewChat(const QByteArray& nByteSource_, const QByteArray& nByteDest_);
    void GetChat(QByteArray& nByteSource_, QByteArray& nByteDest_);

    void GetNeedToProcessMessagesCallback(const NDataStruct::DynArray<Message>& arrMessages_);
    void ErrorCallback(const TcpConnectionPtr&);
    void UpdateShow();
private slots:
    void SendClicked(bool checked_);

private:
    void ChatCallback(int nType_);
    void FormatShowText(const NDataStruct::DynArray<Message>& arrChatMessage_);
private:

    QByteArray m_nByteSource;
    QByteArray m_nByteDest;

    mutable MutexLock m_nMutex;
    Condition m_nCondition;
    int m_nChatRet;
    bool m_bError;

    mutable MutexLock m_nMutexChatMessage;
    NDataStruct::DynArray<Message> m_arrChatMessage;

private:
    Ui::ChatWidget *ui;
};

#endif // CHATWIDGET_H
