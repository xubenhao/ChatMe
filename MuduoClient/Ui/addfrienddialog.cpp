//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "addfrienddialog.h"
#include "ui_addfrienddialog.h"

AddFriendDialog::AddFriendDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddFriendDialog),
    m_nCondition(m_nMutex)
{
    ui->setupUi(this);
    Initialize();
}

void AddFriendDialog::Initialize()
{
    connect(ui->addBtn, SIGNAL(clicked(bool)), this, SLOT(AddClicked(bool)));
    connect(ui->exitBtn, SIGNAL(clicked(bool)), this, SLOT(ExitClicked(bool)));
}

void AddFriendDialog::AddClicked(bool checked_)
{
    QString _strUserId = ui->userIdEdit->text();

    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);
    _pClient->setAddCallback(
        std::bind(&AddFriendDialog::AddCallback, this, std::placeholders::_1));
    _pClient->setErrorCallback(
        std::bind(&AddFriendDialog::ErrorCallback, this, std::placeholders::_1));

    QString _strMaster(m_arrMasterUserId);
    bool _bRet = _pClient->sendAddMessage(_strMaster, _strUserId);
    assert(_bRet);

    m_nMutex.lock();
    m_nAddRet = -1;
    m_bError = false;
    while (m_nAddRet == -1
        && m_bError == false)
    {
      m_nCondition.wait();
    }

    int _nAddRet = m_nAddRet;
    bool _bError = m_bError;
    m_nMutex.unlock();
    if(_bError)
    {
        ui->addRetText->setText(QString("Error"));
    }
    else
    {
        if(_nAddRet == 0)
        {
            ui->addRetText->setText(QString("Fail"));
        }
        else if(_nAddRet == 1)
        {
            ui->addRetText->setText(QString("Success"));
        }
        else
        {
            ui->addRetText->setText(QString("Exception"));
        }
    }
}

void AddFriendDialog::ExitClicked(bool checked_)
{
    QDialog::accept();
}

void AddFriendDialog::AddCallback(int nType_)
{
    MutexLockGuard lock(m_nMutex);
    m_nAddRet = nType_;
    m_nCondition.notify();
}

void AddFriendDialog::ErrorCallback(const TcpConnectionPtr&)
{
    MutexLockGuard lock(m_nMutex);
    m_bError = true;
    m_nCondition.notify();
}

AddFriendDialog::~AddFriendDialog()
{
    delete ui;
}
