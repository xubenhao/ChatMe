//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "registerandloginwidget.h"
#include "ui_registerandloginwidget.h"

RegisterAndLoginWidget::RegisterAndLoginWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterAndLoginWidget),
    m_nCondition(m_nMutex)
{
    ui->setupUi(this);
    Initialize();
}

RegisterAndLoginWidget::~RegisterAndLoginWidget()
{
    delete ui;
}

void RegisterAndLoginWidget::Initialize()
{
    connect(ui->loginBtn, SIGNAL(clicked(bool)), this, SLOT(OnLoginClicked(bool)));
    connect(ui->registerBtn, SIGNAL(clicked(bool)), this, SLOT(OnRegisterClicked(bool)));
}

// how to process login
// as a client
// 1.we can get the login information--userid and password
// 2.we pack and send this to server
// as a server
// 1.we receiver information sended from client
// 2.we parse the information
// we should identify this as a login-info.
// then we get the userid and password from the login-info.
// 3.we as a client run a query to a database server and recever the process result
// we should process the result to identify if the login-info is right a existing user.
// if the login-info is right a existing user, then we should send back to the client to tell it this information.
// if the login-info is not a existing user, then we should send back to client to tell it this information, too.

void RegisterAndLoginWidget::OnLoginClicked(bool bFlag_)
{
    QString _strUserId = ui->userIdEdit->text();
    QString _strPassword = ui->passwordEdit->text();

    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);
    _pClient->setLoginCallback(
        std::bind(&RegisterAndLoginWidget::LoginCallback, this, std::placeholders::_1, std::placeholders::_2));
    _pClient->setErrorCallback(
        std::bind(&RegisterAndLoginWidget::ErrorCallback, this, std::placeholders::_1));

    bool _bRet = _pClient->sendLoginMessage(_strUserId, _strPassword);
    assert(_bRet);

    m_nMutex.lock();
    m_nLoginRet = -1;
    m_bError = false;
    while (m_nLoginRet == -1
        && m_bError == false)
    {
      m_nCondition.wait();
    }

    int _nLoginRet = m_nLoginRet;
    bool _bError = m_bError;
    m_nMutex.unlock();
    if(_bError)
    {
        ui->infoLabel->setText(QString("Error"));
    }
    else
    {
        if(_nLoginRet == 0)
        {
            ui->infoLabel->setText(QString("Fail"));
        }
        else if(_nLoginRet == 1)
        {
            ui->infoLabel->setText(QString("Success"));
            // we should convery the logined userid
            emit ShowMainChatWidget(_strUserId.toUtf8(), m_nTimeStamp);
        }
        else
        {
            ui->infoLabel->setText(QString("Exception"));
        }
    }
}

// how to process register
// as a client
// 1.we can get the register information-userid and password
// 2.we pack and send this to server
// as a server
// 1.we recever information sended from client
// 2.we parse the information
// we should identify this as a register-info.
// then we get the userid and password from the register-info.
// 3.we as a client run a query to a database server and recever the process result
// we should process the result to identify if the register-info has exist or not legal
// if the register-info has existed or is not legal, then we should send back to the client to tell it this information.
// if the register-info is legal and not has existed, then we run a sql to add the register-info into the user table and send back to the client to tell it this information

void RegisterAndLoginWidget::OnRegisterClicked(bool bFlag_)
{
    QString _strUserId = ui->userIdEdit->text();
    QString _strPassword = ui->passwordEdit->text();

    MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
    assert(_pClient);
    // set Register callback to process the register result
    // set Error callback to process error
    // not concern the timeout callback
    _pClient->setRegisterCallback(
        std::bind(&RegisterAndLoginWidget::RegisterCallback, this, std::placeholders::_1));
    _pClient->setErrorCallback(
        std::bind(&RegisterAndLoginWidget::ErrorCallback, this, std::placeholders::_1));

    // pack and send to server
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for register type
    // the content format
    // 1 byte for user-id length, thus we can process a user-id at most 2^8-1 byte
    // 1 byte for password length, thus we can process a password at most 2^8-1 byte
    bool _bRet = _pClient->sendRegisterMessage(_strUserId, _strPassword);
    assert(_bRet);


    // use mutex and condition variable realize the wait and sync between two threads
    // at here
    // 1.we lock+wait condition
    // 2.when the condition is signaled,we continue to process the result
    // 3.when we finish process,release lock
    m_nMutex.lock();
    m_nRegisterRet = -1;
    m_bError = false;
    while (m_nRegisterRet == -1
        && m_bError == false)
    {
      m_nCondition.wait();
    }

    int _nRegisterRet = m_nRegisterRet;
    bool _bError = m_bError;
    m_nMutex.unlock();
    if(_bError)
    {
        ui->infoLabel->setText(QString("Error"));
    }
    else
    {
        if(_nRegisterRet == 0)
        {
            ui->infoLabel->setText(QString("Success"));
        }
        else if(_nRegisterRet == 1)
        {
            ui->infoLabel->setText(QString("Fail"));
        }
        else
        {
            ui->infoLabel->setText(QString("Exception"));
        }
    }
}

void RegisterAndLoginWidget::RegisterCallback(int nType_)
{
    // at here
    // we lock
    // change variable
    // signal condition
    // release lock
    MutexLockGuard lock(m_nMutex);
    m_nRegisterRet = nType_;
    m_nCondition.notify();
}

void RegisterAndLoginWidget::ErrorCallback(const TcpConnectionPtr&)
{
    // Err
    // at here
    // we lock
    // change variable
    // signal condition
    // release lock
    MutexLockGuard lock(m_nMutex);
    m_bError = true;
    m_nCondition.notify();
}

void RegisterAndLoginWidget::LoginCallback(int nType_, TimeStamp nTimeStamp_)
{
    // at here
    // we lock
    // change variable
    // signal condition
    // release lock
    MutexLockGuard lock(m_nMutex);
    m_nLoginRet = nType_;
    m_nTimeStamp = nTimeStamp_;
    m_nCondition.notify();
}

