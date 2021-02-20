//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "chatwidget.h"
#include "ui_chatwidget.h"

ChatWidget::ChatWidget(QWidget *parent) :
    QWidget(parent),
    m_nCondition(m_nMutex),
    ui(new Ui::ChatWidget)
{
    ui->setupUi(this);
    Initialize();
}

void ChatWidget::Initialize()
{
    connect(ui->sendBtn, SIGNAL(clicked(bool)), this, SLOT(SendClicked(bool)));
}

void ChatWidget::ChatCallback(int nType_)
{
    MutexLockGuard lock(m_nMutex);
    m_nChatRet = nType_;
    m_nCondition.notify();
}

void ChatWidget::FormatShowText(const NDataStruct::DynArray<Message>& arrChatMessage_)
{
    char _strChatHistory[1024*100];
    memset(_strChatHistory, 0, sizeof(_strChatHistory));
    char* _pPos = _strChatHistory;
    int _nLeftLength = 1024 * 100;
    for (int i = 0; i < arrChatMessage_.GetSize(); i++)
    {
        char _strDirectionInfo[300];
        memset(_strDirectionInfo, 0, sizeof(_strDirectionInfo));
        if(arrChatMessage_[i].m_bDirection)
        {
            snprintf(_strDirectionInfo, sizeof(_strDirectionInfo), "%s", m_nByteSource.data());
        }
        else
        {
            snprintf(_strDirectionInfo, sizeof(_strDirectionInfo), "\t\t\t%s", m_nByteDest.data());
        }

        int _nNum = snprintf(_pPos, _nLeftLength, "%s:%s\n", _strDirectionInfo, arrChatMessage_[i].m_nContent.data());
        _pPos += _nNum;
        _nLeftLength -= _nNum;
    }

    ui->chatText->setText(_strChatHistory);
}

void ChatWidget::ErrorCallback(const TcpConnectionPtr&)
{
    MutexLockGuard lock(m_nMutex);
    m_bError = true;
    m_nCondition.notify();
}

void ChatWidget::SendClicked(bool checked_)
{
    Q_UNUSED(checked_);
    QString _strText = ui->chatEdit->toPlainText();
    QByteArray _nByteText = _strText.toUtf8();

    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);

    _pClient->setChatCallback(
        std::bind(&ChatWidget::ChatCallback, this, std::placeholders::_1));
    _pClient->setErrorCallback(
        std::bind(&ChatWidget::ErrorCallback, this, std::placeholders::_1));

    bool _bRet = _pClient->sendChatMessage(m_nByteSource, m_nByteDest, _strText);
    assert(_bRet);

    m_nMutex.lock();
    m_nChatRet = -1;
    m_bError = false;
    while (m_nChatRet == -1
        && m_bError == false)
    {
      m_nCondition.wait();
    }

    int _nChatRet = m_nChatRet;
    bool _bError = m_bError;
    m_nMutex.unlock();
    if(_bError)
    {
        assert(false);
    }
    else
    {
        if(_nChatRet == 0)
        {
            Message _nMessage;
            _nMessage.m_nContent = _nByteText;
            _nMessage.m_bDirection = true;
            _nMessage.m_nTimeStamp = TimeStamp::now();
            m_nMutexChatMessage.lock();
            m_arrChatMessage.Add(_nMessage);
            m_nMutexChatMessage.unlock();
            UpdateShow();
        }
    }
}

void ChatWidget::SetNewChat(const QByteArray& nByteSource_, const QByteArray& nByteDest_)
{
    char _strTip[100];
    memset(_strTip, 0, sizeof(_strTip));
    snprintf(_strTip, sizeof(_strTip), "%s to %s chat window", nByteSource_.data(), nByteDest_.data());
    ui->tipLabel->setText(_strTip);
    //setWindowTitle(_strTip);
    m_nByteSource = nByteSource_;
    m_nByteDest = nByteDest_;

}

void ChatWidget::GetNeedToProcessMessagesCallback(const NDataStruct::DynArray<Message>& arrMessages_)
{
    m_nMutexChatMessage.lock();
    for(int i = 0; i < arrMessages_.GetSize(); i++)
    {
        m_arrChatMessage.Add(arrMessages_[i]);
    }
    m_nMutexChatMessage.unlock();
}

void ChatWidget::UpdateShow()
{
    ui->chatEdit->clear();
    m_nMutexChatMessage.lock();
    NDataStruct::DynArray<Message> _arrChatMessage = m_arrChatMessage;
    m_nMutexChatMessage.unlock();
    FormatShowText(_arrChatMessage);
}

void ChatWidget::GetChat(QByteArray& nByteSource_, QByteArray& nByteDest_)
{
    nByteSource_ = m_nByteSource;
    nByteDest_ = m_nByteDest;
}

ChatWidget::~ChatWidget()
{
    delete ui;
}



