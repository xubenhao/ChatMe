//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "mainchatwidget.h"
#include "ui_mainchatwidget.h"
#include "addfrienddialog.h"
#include "chatwidget.h"

MainChatWidget::MainChatWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainChatWidget),
    m_nCondition(m_nMutex),
    m_nConditionForNeedToProcessUsers(m_nMutexForNeedToProcessUsers),
    m_nConditionForGetNeedToProcessMessage(m_nMutexForGetNeedToProcessMessage)
{
    ui->setupUi(this);
    Initialize();
    m_nRefreshTimerId = startTimer(1000);
}

void MainChatWidget::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == m_nRefreshTimerId)
    {
        m_nMutexForNeedToProcessUsers.lock();
        NDataStruct::DynArray<QByteArray> _arrUsers = m_arrNeedToProcessUsers;
        m_nMutexForNeedToProcessUsers.unlock();
        UpdateNeedToProcessUserShow(_arrUsers);
    }
}

void MainChatWidget::UpdateNeedToProcessUserShow(const NDataStruct::DynArray<QByteArray>& arrUsers_)
{
    m_pActiveModel->clear();
    for(int i = 0; i < arrUsers_.GetSize(); i++)
    {
        // make a different show for the need to process user
        QStandardItem* pItem = new QStandardItem();
        pItem->setData(arrUsers_[i], Qt::DisplayRole);
        pItem->setData(arrUsers_[i], Qt::UserRole);
        m_pActiveModel->appendRow(pItem);
    }

    QByteArray _nUserId = m_nChattingUserId;
    int _nIndex = arrUsers_.Find([_nUserId](const QByteArray& nByte_)->bool{
        return (_nUserId == nByte_);
    });
    if(_nIndex == -1)
    {
        return;
    }

    // a send b a message
    // in database b's needtoprocessfriends table insert a

    // master is b
    QString _strSourceUserId = m_arrUserId;
    // dest is a
    QString _strDestUserId = _nUserId;

    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);
    _pClient->setGetNeedToProcessMessagesCallback(
        std::bind(&MainChatWidget::GetNeedToProcessMessagesCallback, this, std::placeholders::_1));
    bool _bRet = _pClient->sendGetNeedToProcessMessagesMessage(_strSourceUserId, _strDestUserId);
    assert(_bRet);

    m_nMutexForGetNeedToProcessMessage.lock();
    m_nGetNeedToProcessMessagesRet = -1;
    m_bError = false;
    while (m_nGetNeedToProcessMessagesRet == -1
        && m_bError == false)
    {
      m_nConditionForGetNeedToProcessMessage.wait();
    }

    m_pChattingWidget->UpdateShow();
    m_nMutexForGetNeedToProcessMessage.unlock();

    m_nMutexForNeedToProcessUsers.lock();
    m_arrNeedToProcessUsers.DeleteByValue(m_nChattingUserId);
    m_nMutexForNeedToProcessUsers.unlock();
}

void MainChatWidget::Initialize()
{
    m_pChattingWidget = nullptr;
    connect(ui->addBtn, SIGNAL(clicked(bool)), this, SLOT(AddClicked(bool)));

    m_pModel = new QStandardItemModel(this);
    ui->friendList->setModel(m_pModel);
    ui->friendList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_pActiveModel = new QStandardItemModel(this);
    ui->needToProcessUsers->setModel(m_pActiveModel);
    ui->needToProcessUsers->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // register signal-slot
    connect(ui->friendList,
        SIGNAL(doubleClicked(QModelIndex)),
        this,
        SLOT(ListDoubleClicked(QModelIndex)));

    ChatWidget* _pWidget = new ChatWidget(this);
    _pWidget->hide();
    _pWidget->SetNewChat(m_arrUserId.toUtf8(), QByteArray(""));
    std::pair<QByteArray, QWidget*> _nPair(QByteArray(""), _pWidget);
    m_arrMapWidget.Add(_nPair);
    ui->widgetLayout->addWidget(_pWidget);
    _pWidget->show();
}

void MainChatWidget::SetUserId(const QString& arrUserId_)
{
    MutexLockGuard lock(m_nMutexLogin);
    m_arrUserId = arrUserId_;
}

void MainChatWidget::SetLoginTime(const TimeStamp& nTimeStamp_)
{
    MutexLockGuard lock(m_nMutexLogin);
    m_nLoginTime = nTimeStamp_;
}

void MainChatWidget::UpdateFriendList()
{
    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);
    _pClient->setUpdateFriendListCallback(
        std::bind(&MainChatWidget::UpdateFriendCallback, this, std::placeholders::_1));
    //_pClient->setErrorCallback(
    //    std::bind(&MainChatWidget::ErrorCallback, this, std::placeholders::_1));

    bool _bRet = _pClient->sendUpdateFriendListMessage(m_arrUserId);
    assert(_bRet);

    m_nMutex.lock();
    m_arrFriends.DeleteAll();
    m_bFriendValid = false;
    m_bError = false;
    while (m_bFriendValid == false
        && m_bError == false)
    {
      m_nCondition.wait();
    }

    NDataStruct::DynArray<QByteArray> _arrFriends = m_arrFriends;
    bool _bError = m_bError;
    m_nMutex.unlock();
    if(_bError)
    {
        assert(false);
    }
    else
    {
        m_pModel->clear();
        for(int i = 0; i < _arrFriends.GetSize(); i++)
        {
            QStandardItem* pItem = new QStandardItem();
            pItem->setData(_arrFriends[i], Qt::DisplayRole);
            pItem->setData(_arrFriends[i], Qt::UserRole);
            m_pModel->appendRow(pItem);
        }
    }
}

void MainChatWidget::NeedToProcessUserList()
{
    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);
    _pClient->setNeedToProcessUserListCallback(
        std::bind(&MainChatWidget::NeedToProcessUserListCallback, this, std::placeholders::_1));
    //_pClient->setErrorCallback(
    //    std::bind(&MainChatWidget::ErrorCallback, this, std::placeholders::_1));

    bool _bRet = _pClient->sendNeedToProcessUserListMessage(m_arrUserId);
    assert(_bRet);

    m_nMutexForNeedToProcessUsers.lock();
    //m_arrNeedToProcessUsers.DeleteAll();
    m_bNeedToProcessUserValid = false;
    m_bError = false;
    while (m_bNeedToProcessUserValid == false
        && m_bError == false)
    {
      m_nConditionForNeedToProcessUsers.wait();
    }

    assert(m_bError == false);
    m_nMutexForNeedToProcessUsers.unlock();
}

void MainChatWidget::ErrorCallback(const TcpConnectionPtr&)
{
    assert(false);

}

void MainChatWidget::GetNeedToProcessMessagesCallback(const NDataStruct::DynArray<Message>& arrMessages_)
{
    MutexLockGuard lock(m_nMutexForGetNeedToProcessMessage);
    if(m_pChattingWidget)
    {
        m_pChattingWidget->GetNeedToProcessMessagesCallback(arrMessages_);
    }

    m_nGetNeedToProcessMessagesRet = 0;
    m_nConditionForGetNeedToProcessMessage.notify();
}

void MainChatWidget::NewMessageCallback(const QByteArray& nByteSender_)
{
    m_nMutexForNeedToProcessUsers.lock();
    int _nIndex = m_arrNeedToProcessUsers.Find([nByteSender_](const QByteArray& nByteUser_)->bool {
        return (nByteUser_ == nByteSender_);
    });
    if(_nIndex == -1)
    {
        m_arrNeedToProcessUsers.Add(nByteSender_);
    }

    m_nMutexForNeedToProcessUsers.unlock();
}

void MainChatWidget::NeedToProcessUserListCallback(const NDataStruct::DynArray<QByteArray>& arrUserIds_)
{
    MutexLockGuard lock(m_nMutexForNeedToProcessUsers);
    m_arrNeedToProcessUsers = arrUserIds_;
    m_bNeedToProcessUserValid = true;
    m_nConditionForNeedToProcessUsers.notify();
}

void MainChatWidget::UpdateFriendCallback(const NDataStruct::DynArray<QByteArray>& arrUserIds_)
{
    MutexLockGuard lock(m_nMutex);
    m_arrFriends = arrUserIds_;
    m_bFriendValid = true;
    m_nCondition.notify();
}

void MainChatWidget::ListDoubleClicked(const QModelIndex& index)
{
    QVariant var = m_pModel->data(index, Qt::UserRole);
    int i = 0;
    for(; i < m_arrMapWidget.GetSize(); i++)
    {
        if(m_arrMapWidget[i].first == var.toByteArray())
        {
            break;
        }
    }

    if(i == m_arrMapWidget.GetSize())
    {
        ChatWidget* _pWidget = new ChatWidget(this);
        _pWidget->hide();
        _pWidget->SetNewChat(m_arrUserId.toUtf8(), var.toByteArray());
        std::pair<QByteArray, QWidget*> _nPair(QByteArray(var.toByteArray()), _pWidget);
        m_arrMapWidget.Add(_nPair);
        ui->widgetLayout->addWidget(_pWidget);
    }

    for(int j = 0; j < m_arrMapWidget.GetSize(); j++)
    {
        if(j != i)
        {
            m_arrMapWidget[j].second->hide();
        }
        else
        {
            m_pChattingWidget = (ChatWidget*)(m_arrMapWidget[j].second);
            m_nChattingUserId = m_arrMapWidget[j].first;
            m_arrMapWidget[j].second->show();
        }
    }
}

void MainChatWidget::AddClicked(bool checked_)
{
    Q_UNUSED(checked_);
    AddFriendDialog *_pAddFriendDialog = new AddFriendDialog(this);
    _pAddFriendDialog->SetMasterUserId(m_arrUserId.toUtf8());
    int _nRet = _pAddFriendDialog->exec();
    if(_nRet == QDialog::Accepted)
    {
        UpdateFriendList();
    }

    delete _pAddFriendDialog;
}

MainChatWidget::~MainChatWidget()
{
    delete ui;
}


void MainChatWidget::OffLine()
{
    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);
    bool _bRet = _pClient->sendOffLineMessage(m_arrUserId, m_nLoginTime);
    assert(_bRet);
    // just send the notification, not need to wait for res.
}






















