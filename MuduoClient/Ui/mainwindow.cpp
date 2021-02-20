//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "registerandloginwidget.h"
#include "mainchatwidget.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    Initialize();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QByteArray _nByteArr("MainChat");
    for(int i = 0; i < m_arrMapWidget.GetSize(); i++)
    {
        if(_nByteArr == m_arrMapWidget[i].first)
        {
            MainChatWidget* _pWidget =  (MainChatWidget*)(m_arrMapWidget[i].second);
            _pWidget->OffLine();
            break;
        }

    }
}

void MainWindow::Initialize()
{
    RegisterAndLoginWidget* _pWidget = new RegisterAndLoginWidget(this);
    ui->horizontalLayout->addWidget(_pWidget);

    std::pair<QByteArray, QWidget*> _nPair;
    _nPair.first = QByteArray("RegisterAndLogin");
    _nPair.second = _pWidget;
    m_arrMapWidget.Add(_nPair);

    connect(
        _pWidget,
        SIGNAL(ShowMainChatWidget(const QByteArray&, const TimeStamp&)),
        this,
        SLOT(ShowMainChatWidget(const QByteArray&, const TimeStamp&)));

    MainChatWidget* _pChatWidget = new MainChatWidget(this);
    _pChatWidget->hide();
    ui->horizontalLayout->addWidget(_pChatWidget);

    _nPair.first = QByteArray("MainChat");
    _nPair.second = _pChatWidget;
    m_arrMapWidget.Add(_nPair);

    connect(
        _pChatWidget,
        SIGNAL(ShowDataStruct(const QString&)),
        this,
        SLOT(ShowDataStruct(const QString&)));
}

void MainWindow::ShowDataStruct(const QString& strName_)
{
    QByteArray _nByteArr = strName_.toUtf8();
    for(int i = 0; i < m_arrMapWidget.GetSize(); i++)
    {
        if(_nByteArr != m_arrMapWidget[i].first)
        {
            m_arrMapWidget[i].second->hide();
        }
        else
        {
            m_arrMapWidget[i].second->show();
        }
    }
}

void MainWindow::ShowMainChatWidget(const QByteArray& strUserId_, const TimeStamp& nTimeStamp_)
{
    QByteArray _nByteArr("MainChat");
    for(int i = 0; i < m_arrMapWidget.GetSize(); i++)
    {
        if(_nByteArr != m_arrMapWidget[i].first)
        {
            m_arrMapWidget[i].second->hide();
        }
        else
        {
            MainChatWidget* _pWidget =  (MainChatWidget*)(m_arrMapWidget[i].second);
            //_pWidget->
            // this is the right place to set new-message-callback
            MuduoClient* _pClient = MuduoClient::instance(nullptr, InetAddress());
            assert(_pClient);
            _pClient->setNewMessageCallback(
                std::bind(&MainChatWidget::NewMessageCallback, _pWidget, std::placeholders::_1));

            _pWidget->SetUserId(strUserId_);
            _pWidget->SetLoginTime(nTimeStamp_);
            _pWidget->UpdateFriendList();
            _pWidget->NeedToProcessUserList();
            m_arrMapWidget[i].second->show();
        }
    }
}
