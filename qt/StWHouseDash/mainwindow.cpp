#include "mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QSqlError>
#include <QMessageBox>
#include <QListWidget>
#include <QComboBox>
#include <QLinearGradient>
#include <QPair>
#include <QGridLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_chartXCount(0), m_doorOpen(false), m_isHistoryMode(false), m_lastHistoryHours(1)
{
    // ==========================================
    // 1. 全局顶级 UI 质感换肤（企业级大屏分辨率与 QSS）?
    // ==========================================
    this->setWindowTitle("仓瞳智能仓储分布式大屏");
    this->resize(1600, 900);
    this->setMinimumSize(1400, 800);
    this->setStyleSheet(
        // 全局底色
        "QMainWindow { background: #0a0f1a; }"
        // 卡片：深色底+细边框+8px圆角
        "QFrame { background: #141c2b; border: 1px solid #1e2d45; border-radius: 8px; }"
        // 标签：统一字体
        "QLabel { color: #c8cdd4; font-family: Microsoft YaHei; }"
        // 按钮：深灰底、hover变蓝
        "QPushButton { background: #1a2540; border: 1px solid #253555; border-radius: 6px; color: #c8cdd4; padding: 6px 14px; font-family: Microsoft YaHei; font-size: 12px; }"
        "QPushButton:hover { background: #1d3a6b; border-color: #2563eb; color: #ffffff; }"
        "QPushButton:pressed { background: #15294f; }"
        // 滑轨
        "QSlider::groove:horizontal { height: 4px; background: #1a2540; border-radius: 2px; border: none; }"
        "QSlider::sub-page:horizontal { background: #2563eb; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #e2e8f0; border: 2px solid #3b82f6; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: #ffffff; border-color: #60a5fa; }"
    );

    QWidget *centralWidget = new QWidget(this);

           // 增加顶部标题栏结构?
    QVBoxLayout *mainLayout = new QVBoxLayout();

    QFrame *header = new QFrame();
    header->setFixedHeight(80);

    QHBoxLayout *headerLayout = new QHBoxLayout(header);

    QLabel *title = new QLabel("仓瞳智慧仓储数字孪生平台");
    title->setStyleSheet(
        "font-size:22px;"
        "font-weight:700;"
        "color:white;"
        );
    mqttStatusLabel = new QLabel("● 物联网络: 等待连接...");
    mqttStatusLabel->setStyleSheet(
        "font-size:14px;"
        "font-weight:bold;"
        "color:#EF4444;"
        );

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(mqttStatusLabel);

    mainLayout->addWidget(header);

           // 修改主布局嵌套关系
    QHBoxLayout *rootLayout = new QHBoxLayout();
    mainLayout->addLayout(rootLayout);
    centralWidget->setLayout(mainLayout);

           // ==========================================
           // 左侧：控制与核心指标面板 (Width: 300 缩窄)
           // ==========================================
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(300);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);

           // [系统工具栏]
    QHBoxLayout *sysToolLayout = new QHBoxLayout();
    manageUsersBtn = new QPushButton("👥 账号管理中心", this);
    cardMgrBtn = new QPushButton("💳 卡号管理", this);
    logoutBtn = new QPushButton("🚪 退出登录", this);
    sysToolLayout->addWidget(manageUsersBtn);
    sysToolLayout->addWidget(cardMgrBtn);
    sysToolLayout->addStretch();
    sysToolLayout->addWidget(logoutBtn);
    leftLayout->addLayout(sysToolLayout);

           // [指标    // [指标状态卡片 - 升级为 2x2 网格架构并进行汉化]
    QFrame *metricCard = new QFrame(this);
    QVBoxLayout *metricLayout = new QVBoxLayout(metricCard);
    metricLayout->setContentsMargins(15, 15, 15, 15);
    metricLayout->setSpacing(12);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12);

           //     // 2x2指标卡片动态 Lambda 构建器
    auto createCard = [this](QString title, QString color, QLabel*& valueLabel)
    {
        QFrame *card = new QFrame();
        QVBoxLayout *layout = new QVBoxLayout(card);

        QLabel *name = new QLabel(title);
        name->setStyleSheet(
            "font-size:11px;"
            "color:#64748b;"
            "font-weight:600;"
            "letter-spacing:0.5px;"
            );

        valueLabel = new QLabel("--");
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setStyleSheet(
            QString(
                "font-size:28px;"
                "font-weight:700;"
                "color:%1;"
                "font-family:\"Segoe UI\",\"Consolas\",sans-serif;"
                ).arg(color)
            );

        layout->addWidget(name);
        layout->addWidget(valueLabel);
        return card;
    };

    grid->addWidget(createCard("🌡️ 环境温度", "#EF4444", tempValueLabel), 0, 0);
    grid->addWidget(createCard("💧 环境湿度", "#06B6D4", humiValueLabel), 0, 1);
    grid->addWidget(createCard("💨 烟雾浓度", "#F59E0B", smokeValueLabel), 1, 0);
    grid->addWidget(createCard("☀️ 光照强度", "#8B5CF6", lightValueLabel), 1, 1);

    metricLayout->addLayout(grid);

           // LED 亮度小字显示持续保留并挂载到网格下方
    ledBrightnessLabel = new QLabel("💡 LED状态 --%", this);
    ledBrightnessLabel->setStyleSheet("font-size: 12px; color: #eab308; padding-left: 5px; border:none; background:transparent; font-weight:bold;");
    metricLayout->addWidget(ledBrightnessLabel);

    leftLayout->addWidget(metricCard);

           // [    // [安防状态防御阵线 - 升级工业灯样式并全面汉化]
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(10);
    QLabel *pirStatus = new QLabel("红外安防\n--", this);
    QLabel *flmStatus = new QLabel("火焰高危\n--", this);
    QLabel *srvStatus = new QLabel("仓门状态\n--", this);

    QString safeStyle = 
        "background:#052E16; border:1.5px solid #22c55e; border-radius:6px; font-size:12px; font-weight:bold; color:#bbf7d0;";

    pirStatus->setStyleSheet(safeStyle); pirStatus->setAlignment(Qt::AlignCenter);
    flmStatus->setStyleSheet(safeStyle); flmStatus->setAlignment(Qt::AlignCenter);
    srvStatus->setStyleSheet(safeStyle); srvStatus->setAlignment(Qt::AlignCenter);
    statusLayout->addWidget(pirStatus); statusLayout->addWidget(flmStatus); statusLayout->addWidget(srvStatus);
    leftLayout->addLayout(statusLayout);

    this->setProperty("pir_lbl", QVariant::fromValue(pirStatus));
    this->setProperty("flm_lbl", QVariant::fromValue(flmStatus));
    this->setProperty("srv_lbl", QVariant::fromValue(srvStatus));

           // [智能控制中心卡片]
    QFrame *ctrlCard = new QFrame(this);
    QVBoxLayout *ctrlLayout = new QVBoxLayout(ctrlCard);
    ctrlLayout->setContentsMargins(18, 15, 18, 15);
    ctrlLayout->setSpacing(15);

    QLabel *ctrlTitle = new QLabel("⚙️ 智能控制中心", this);
    ctrlTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #475569; border:none; background:transparent;");
    ctrlLayout->addWidget(ctrlTitle);

    QString iosToggleStyle = "QPushButton { background-color: #1a2540; border:1px solid #253555; border-radius:12px; }"
                             "QPushButton:checked { background-color: #2563eb; border-color: #3b82f6; }";

           // 排风机控制行
    QHBoxLayout *fanRow = new QHBoxLayout();
    QLabel *fanName = new QLabel("🌀 主排风机", this);
    fanName->setStyleSheet("font-size: 13px; font-weight: bold; border:none; background:transparent; color:#cbd5e1;");
    fanBtn = new QPushButton(this); fanBtn->setCheckable(true); fanBtn->setFixedSize(44, 24); fanBtn->setStyleSheet(iosToggleStyle);
    fanSlider = new QSlider(Qt::Horizontal, this); fanSlider->setRange(0, 100);
    fanRow->addWidget(fanName); fanRow->addWidget(fanBtn); fanRow->addSpacing(5); fanRow->addWidget(fanSlider);
    ctrlLayout->addLayout(fanRow);

           //     // 调光系统：
    QHBoxLayout *ledRow = new QHBoxLayout();
    QLabel *ledName = new QLabel("🌙 调光系统", this);
    ledName->setStyleSheet("font-size: 13px; font-weight: bold; border:none; background:transparent; color:#cbd5e1;");
    lockBtn = new QPushButton(this); lockBtn->setCheckable(true); lockBtn->setFixedSize(44, 24); lockBtn->setStyleSheet(iosToggleStyle);
    ledSlider = new QSlider(Qt::Horizontal, this); ledSlider->setRange(0, 100);
    ledRow->addWidget(ledName); ledRow->addWidget(lockBtn); ledRow->addSpacing(5); ledRow->addWidget(ledSlider);
    ctrlLayout->addLayout(ledRow);

    leftLayout->addWidget(ctrlCard);

    // ??????
    QHBoxLayout *doorRow = new QHBoxLayout();
    QLabel *doorName = new QLabel("🚪 远程开关门", this);
    doorName->setStyleSheet("font-size: 13px; font-weight: bold;");
    doorBtn = new QPushButton("关门", this); doorBtn->setFixedSize(56, 28); doorBtn->setStyleSheet("QPushButton { background-color: #16a34a; color: #fff; border-radius: 6px; font-weight: bold; } QPushButton:hover { background-color: #15803d; }");
    doorRow->addWidget(doorName);
    doorRow->addStretch();
    doorRow->addWidget(doorBtn);
    ctrlLayout->addLayout(doorRow);

    leftLayout->addWidget(ctrlCard);
    rootLayout->addWidget(leftPanel);

           // ==========================================
           // 右侧：物联网大屏分布式图表空间
           // ==========================================
    setupChart();

    // 实时/历史 切换栏
    QHBoxLayout *chartToggleLayout = new QHBoxLayout();
    m_btnRealtime = new QPushButton("📡 实时监控", this);
    m_btnHistory = new QPushButton("📊 历史回溯", this);
    m_btnRealtime->setCheckable(true); m_btnRealtime->setChecked(true);
    m_btnHistory->setCheckable(true);
    QString toggleStyle = "QPushButton { padding: 8px 20px; font-size: 13px; font-weight: bold; }"
                          "QPushButton:checked { background: #2563eb; border-color: #3b82f6; color: #fff; }";
    m_btnRealtime->setStyleSheet(toggleStyle);
    m_btnHistory->setStyleSheet(toggleStyle);

    connect(m_btnRealtime, &QPushButton::clicked, this, [this]() {
        m_btnRealtime->setChecked(true); m_btnHistory->setChecked(false);
        switchToRealtime();
    });
    connect(m_btnHistory, &QPushButton::clicked, this, [this]() {
        m_btnHistory->setChecked(true); m_btnRealtime->setChecked(false);
        m_historyBar->setVisible(true);
    });

    chartToggleLayout->addWidget(m_btnRealtime);
    chartToggleLayout->addWidget(m_btnHistory);
    chartToggleLayout->addStretch();

    // 历史时间快捷栏（默认隐藏）
    m_historyBar = new QWidget(this);
    QHBoxLayout *histBarLayout = new QHBoxLayout(m_historyBar);
    histBarLayout->setContentsMargins(0, 0, 0, 0);
    m_btnH1h  = new QPushButton("最近1小时", this);
    m_btnH6h  = new QPushButton("最近6小时", this);
    m_btnH24h = new QPushButton("最近24小时", this);
    QString histBtnStyle = "QPushButton { padding: 6px 14px; font-size: 12px; }";
    m_btnH1h->setStyleSheet(histBtnStyle); m_btnH6h->setStyleSheet(histBtnStyle); m_btnH24h->setStyleSheet(histBtnStyle);
    histBarLayout->addWidget(m_btnH1h);
    histBarLayout->addWidget(m_btnH6h);
    histBarLayout->addWidget(m_btnH24h);
    histBarLayout->addStretch();
    QPushButton *clearBtn = new QPushButton("🗑 一键清空", this);
    clearBtn->setStyleSheet("QPushButton { padding: 6px 14px; font-size: 12px; background:#7f1d1d; color:#fecaca; border:1px solid #991b1b; }"
                            "QPushButton:hover { background:#991b1b; border-color:#ef4444; color:#fff; }");
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton btn = QMessageBox::question(this, "确认清空",
            "确定要删除全部历史数据吗？此操作不可恢复。", QMessageBox::Yes | QMessageBox::No);
        if (btn == QMessageBox::Yes) {
            QSqlQuery q;
            q.exec("DELETE FROM sensor_logs");
            qDebug() << "[DB] 历史数据已一键清空";
            loadHistory(m_lastHistoryHours);
        }
    });
    histBarLayout->addWidget(clearBtn);
    connect(m_btnH1h,  &QPushButton::clicked, this, [this]() { loadHistory(1); });
    connect(m_btnH6h,  &QPushButton::clicked, this, [this]() { loadHistory(6); });
    connect(m_btnH24h, &QPushButton::clicked, this, [this]() { loadHistory(24); });
    m_historyBar->setVisible(false);

    // QStackedWidget: 0=实时图表, 1=历史面板
    m_chartStack = new QStackedWidget(this);
    m_chartStack->addWidget(chartView); // page 0

    // ====== 历史面板: 摘要卡片 + 表格 ======
    m_historyPanel = new QWidget(this);
    QVBoxLayout *histPanelLayout = new QVBoxLayout(m_historyPanel);
    histPanelLayout->setContentsMargins(0, 8, 0, 0);
    histPanelLayout->setSpacing(12);

    // 摘要卡片行
    QHBoxLayout *summaryRow = new QHBoxLayout();
    summaryRow->setSpacing(12);
    auto makeSummaryCard = [this](const QString &title, QLabel *&valLabel) {
        QFrame *card = new QFrame(this);
        card->setStyleSheet("QFrame { background:#141c2b; border:1px solid #1e2d45; border-radius:8px; }");
        QVBoxLayout *lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 10, 14, 10);
        QLabel *t = new QLabel(title, this);
        t->setStyleSheet("font-size:11px; color:#64748b; font-weight:600; border:none; background:transparent;");
        valLabel = new QLabel("--", this);
        valLabel->setStyleSheet("font-size:20px; font-weight:700; color:#f8fafc; border:none; background:transparent;");
        lay->addWidget(t);
        lay->addWidget(valLabel);
        return card;
    };
    summaryRow->addWidget(makeSummaryCard("🌡 平均温度", m_histAvgTemp));
    summaryRow->addWidget(makeSummaryCard("🔺 最高温度", m_histMaxTemp));
    summaryRow->addWidget(makeSummaryCard("🔻 最低温度", m_histMinTemp));
    summaryRow->addWidget(makeSummaryCard("💧 平均湿度", m_histAvgHumi));
    summaryRow->addWidget(makeSummaryCard("🔺 最高湿度", m_histMaxHumi));
    summaryRow->addWidget(makeSummaryCard("🔻 最低湿度", m_histMinHumi));
    summaryRow->addWidget(makeSummaryCard("📋 采样条数", m_histSampleCount));
    histPanelLayout->addLayout(summaryRow);

    // 数据表格
    m_histTable = new QTableWidget(this);
    m_histTable->setColumnCount(7);
    m_histTable->setHorizontalHeaderLabels({"时间", "温度℃", "湿度%", "烟雾", "光照", "风扇%", "LED%"});
    m_histTable->horizontalHeader()->setStretchLastSection(true);
    m_histTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_histTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_histTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_histTable->setAlternatingRowColors(true);
    m_histTable->setStyleSheet(
        "QTableWidget { background:#141c2b; border:1px solid #1e2d45; border-radius:8px; color:#cbd5e1; "
        "gridline-color:#1e2d45; font-size:12px; }"
        "QTableWidget::item { padding:4px 8px; }"
        "QTableWidget::item:selected { background:#1d3a6b; color:#fff; }"
        "QHeaderView::section { background:#0f172a; color:#94a3b8; border:1px solid #1e2d45; "
        "padding:6px; font-weight:bold; font-size:11px; }"
    );
    histPanelLayout->addWidget(m_histTable, 1);

    m_chartStack->addWidget(m_historyPanel); // page 1

    QVBoxLayout *chartArea = new QVBoxLayout();
    chartArea->addLayout(chartToggleLayout);
    chartArea->addWidget(m_historyBar);
    chartArea->addWidget(m_chartStack, 1);
    rootLayout->addLayout(chartArea, 1);

    setCentralWidget(centralWidget);

           // ==========================================
           //     // 通信总线联动与事件绑定
           // ==========================================
    m_mqttClient = new SimpleMqttClient(this);
    connect(m_mqttClient, &SimpleMqttClient::connected, this, &MainWindow::onMqttConnected);
    connect(m_mqttClient, &SimpleMqttClient::messageReceived, this, &MainWindow::handleMqttMessage);
    connect(m_mqttClient, &SimpleMqttClient::disconnected, this, &MainWindow::onMqttDisconnected);
    m_mqttClient->connectToBroker("192.168.123.59", 1883);

    connect(fanBtn, &QPushButton::clicked, this, [=](bool checked){
        fanSlider->blockSignals(true); fanSlider->setValue(checked ? 50 : 0); fanSlider->blockSignals(false);
        m_mqttClient->publishMessage("wms/warehouse1/cmd", checked ? "{\"speed\":50}" : "{\"speed\":0}");
    });
    connect(fanSlider, &QSlider::valueChanged, this, [=](int value){
        QJsonObject obj; obj["speed"] = value; QJsonDocument doc(obj);
        m_mqttClient->publishMessage("wms/warehouse1/cmd", doc.toJson(QJsonDocument::Compact));
        fanBtn->blockSignals(true); fanBtn->setChecked(value > 0); fanBtn->blockSignals(false);
    });
    connect(lockBtn, &QPushButton::clicked, this, [=](bool checked){
        m_mqttClient->publishMessage("wms/warehouse1/cmd", checked ? "{\"led_mode\":\"manual\"}" : "{\"led_mode\":\"auto\"}");
    });
    connect(ledSlider, &QSlider::valueChanged, this, [=](int value){
        m_mqttClient->publishMessage("wms/warehouse1/cmd", "{\"led_mode\":\"manual\"}");
        QJsonObject obj; obj["led_brightness"] = value; QJsonDocument doc(obj);
        m_mqttClient->publishMessage("wms/warehouse1/cmd", doc.toJson(QJsonDocument::Compact));
        lockBtn->blockSignals(true); lockBtn->setChecked(true); lockBtn->blockSignals(false);
    });

    // ??????
    connect(doorBtn, &QPushButton::clicked, this, [this](){
        // toggle: send opposite of current state
        m_mqttClient->publishMessage("wms/warehouse1/cmd", m_doorOpen ? "{\"servo\":0}" : "{\"servo\":1}");
    });

    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(manageUsersBtn, &QPushButton::clicked, this, &MainWindow::onManageUsersClicked);

    // 卡号管理窗口
    connect(cardMgrBtn, &QPushButton::clicked, this, [this]() {
        QDialog dlg(this);
        dlg.setWindowTitle("卡号授权管理中心");
        dlg.resize(520, 400);
        dlg.setStyleSheet("QDialog { background-color: #0f172a; } QLabel { color: #94a3b8; }");

        QVBoxLayout *lay = new QVBoxLayout(&dlg);
        QLabel *title = new QLabel("💳 已检测到的卡号列表", &dlg);
        title->setStyleSheet("font-size:14px; font-weight:bold; color:#f8fafc; margin-bottom:8px;");
        lay->addWidget(title);

        QListWidget *cardList = new QListWidget(&dlg);
        cardList->setStyleSheet("background: #1e293b; color: #fff; border-radius: 6px; font-family: Consolas;");

        auto refreshCards = [this, cardList]() {
            cardList->clear();
            QSqlQuery q("SELECT uid, label, authorized, created_at FROM cards ORDER BY created_at DESC");
            while (q.next()) {
                QString uid = q.value(0).toString();
                int auth = q.value(2).toInt();
                QString icon = auth ? "🟢" : "🔴";
                QString status = auth ? "[已授权]" : "[未授权]";
                auto *listItem = new QListWidgetItem(QString("%1 %2 %3  %4").arg(icon, uid, status, q.value(3).toString())); listItem->setData(Qt::UserRole, uid); cardList->addItem(listItem);
            }
        };
        refreshCards();
        lay->addWidget(cardList);

        // ???????? rfid ???????
        m_mqttClient->publishMessage("wms/warehouse1/rfid", "{\"uid\":\"TEST1234\"}");
        qDebug() << "[RFID] Sent test message to rfid topic";

        QHBoxLayout *btnRow = new QHBoxLayout();
        QPushButton *authBtn = new QPushButton("✅ 授权开门", &dlg);
        QPushButton *unauthBtn = new QPushButton("❌ 取消授权", &dlg);
        QPushButton *clearBtn = new QPushButton("🗑 清空全部", &dlg);
        QPushButton *delCardBtn = new QPushButton("❌ 删除卡号", &dlg);
        QString btnStyle = "QPushButton { background-color: #1e293b; color: #fff; border-radius: 6px; padding: 8px 16px; font-weight:bold; }"
                           "QPushButton:hover { background-color: #2563eb; }";
        authBtn->setStyleSheet(btnStyle + "QPushButton:hover { background-color: #16a34a; }");
        unauthBtn->setStyleSheet(btnStyle + "QPushButton:hover { background-color: #dc2626; }");
        clearBtn->setStyleSheet(btnStyle + "QPushButton:hover { background-color: #dc2626; }");
        delCardBtn->setStyleSheet(btnStyle + "QPushButton:hover { background-color: #dc2626; }");
        btnRow->addWidget(authBtn);
        btnRow->addWidget(unauthBtn);
        btnRow->addWidget(clearBtn);
        btnRow->addWidget(delCardBtn);
        lay->addLayout(btnRow);

        // 授权
        connect(authBtn, &QPushButton::clicked, this, [this, cardList, refreshCards]() {
            auto item = cardList->currentItem();
            if (!item) return;
            QString uid = item->data(Qt::UserRole).toString();
            if (uid.isEmpty()) return;
            QSqlQuery q;
            q.prepare("UPDATE cards SET authorized=1 WHERE uid=:uid");
            q.bindValue(":uid", uid);
            q.exec();
            // 发送授权命令到 STM32
            m_mqttClient->publishMessage("wms/warehouse1/cmd",
                QByteArray("{\"auth_add\":\"" + uid.toUtf8() + "\"}") );
            refreshCards();
        });

        // 取消授权
        connect(unauthBtn, &QPushButton::clicked, this, [this, cardList, refreshCards]() {
            auto item = cardList->currentItem();
            if (!item) return;
            QString uid = item->data(Qt::UserRole).toString();
            if (uid.isEmpty()) return;
            QSqlQuery q;
            q.prepare("UPDATE cards SET authorized=0 WHERE uid=:uid");
            q.bindValue(":uid", uid);
            q.exec();
            m_mqttClient->publishMessage("wms/warehouse1/cmd",
                QByteArray("{\"auth_del\":\"" + uid.toUtf8() + "\"}") );
            refreshCards();
        });

        // 清空
        connect(clearBtn, &QPushButton::clicked, this, [this, cardList, refreshCards]() {
            QSqlQuery q("UPDATE cards SET authorized=0");
            q.exec();
            m_mqttClient->publishMessage("wms/warehouse1/cmd", "{\"auth_clr\":1}");
            refreshCards();
        });


        // 删除卡号
        connect(delCardBtn, &QPushButton::clicked, this, [this, cardList, refreshCards]() {
            auto item = cardList->currentItem();
            if (!item) return;
            QString uid = item->data(Qt::UserRole).toString();
            if (uid.isEmpty()) return;
            // 从数据库删除
            QSqlQuery q;
            q.prepare("DELETE FROM cards WHERE uid=:uid");
            q.bindValue(":uid", uid);
            q.exec();
            // 同步取消 STM32 授权
            m_mqttClient->publishMessage("wms/warehouse1/cmd",
                QByteArray("{\"auth_del\":\"" + uid.toUtf8() + "\"}") );
            refreshCards();
        });
        dlg.exec();
    });

           // 初始化本地存储结构?
    initLocalDatabase();

    // sensor_logs 10秒定时写入
    m_logTimer = new QTimer(this);
    connect(m_logTimer, &QTimer::timeout, this, &MainWindow::flushSensorLogs);
    m_logTimer->start(10000);

    // 历史面板自动刷新定时器
    m_histRefreshTimer = new QTimer(this);
    connect(m_histRefreshTimer, &QTimer::timeout, this, [this]() {
        if (m_isHistoryMode) loadHistory(m_lastHistoryHours);
    });
}

// =========================================================================
// 📊 初始化高级数据科技图表画布（彻底升级工业数字孪生风格）
// =========================================================================
void MainWindow::setupChart()
{
    QChart *chart = new QChart();
    // 手动设色，不用 ChartThemeDark
    chart->setTitle("环境监测实时趋势分析");
    chart->setTitleFont(QFont("Microsoft YaHei", 15, QFont::Bold));
    chart->setTitleBrush(QBrush(QColor("#f8fafc")));
    chart->setBackgroundBrush(QColor("#0a0f1a"));
    chart->setPlotAreaBackgroundBrush(QColor("#141c2b"));
    chart->setPlotAreaBackgroundVisible(true);

           // 初始化三组数据曲线（    // 初始化三组数据曲线（加粗 3px 动感霓虹线条）
    tempSeries = new QLineSeries();  tempSeries->setName("温度 (℃)");       tempSeries->setPen(QPen(QColor("#EF4444"), 2.5));
    humiSeries = new QLineSeries();  humiSeries->setName("湿度 (%)");        humiSeries->setPen(QPen(QColor("#06B6D4"), 2.5));
    smokeSeries = new QLineSeries(); smokeSeries->setName("烟雾浓度 (x10)");  smokeSeries->setPen(QPen(QColor("#F59E0B"), 2.5));

    chart->addSeries(tempSeries);
    chart->addSeries(humiSeries);
    chart->addSeries(smokeSeries);

           // 配置坐标轴?
    axisX = new QValueAxis();
    axisX->setTitleText("时间");
    axisX->setTitleFont(QFont("Microsoft YaHei", 10));
    axisX->setLabelFormat("%d");
    axisX->setRange(0, 30);
    axisX->setLabelsColor(QColor("#94a3b8"));
    axisX->setLabelsFont(QFont("Consolas", 9));
    axisX->setGridLineColor(QColor(255,255,255,12));

    axisY = new QValueAxis();
    axisY->setTitleText("值");
    axisY->setTitleFont(QFont("Microsoft YaHei", 10));
    axisY->setRange(0, 100);
    axisY->setLabelsColor(QColor("#94a3b8"));
    axisY->setLabelsFont(QFont("Consolas", 9));
    axisY->setGridLineColor(QColor(255,255,255,12));

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    tempSeries->attachAxis(axisX);  tempSeries->attachAxis(axisY);
    humiSeries->attachAxis(axisX);  humiSeries->attachAxis(axisY);
    smokeSeries->attachAxis(axisX); smokeSeries->attachAxis(axisY);

           // 设置图例颜色
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->legend()->setLabelColor(QColor("#cbd5e1"));
    chart->legend()->setFont(QFont("Segoe UI", 10));
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("border: 1px solid #1e2d45; border-radius: 8px; background: #141c2b;");
}

// =========================================================================
// 📡 // 📡 核心通信流：处理下位机数据、执行5点滑动平均滤波、刷新图表
// =========================================================================
void MainWindow::handleMqttMessage(const QString &topic, const QByteArray &payload)
{
    qDebug() << "【大屏收到数据】Topic:" << topic << "Payload:" << payload;

    if (topic != "wms/warehouse1/data" && topic != "wms/warehouse1/rfid") return;

    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (doc.isNull() || !doc.isObject()) return;
    QJsonObject obj = doc.object();

           // 1. 先    // 1. 先实时更新左侧 2x2 面板的数值显示（最快反馈）
    if (obj.contains("temp")) tempValueLabel->setText(QString::number(obj["temp"].toDouble(), 'f', 1));
    if (obj.contains("humi")) humiValueLabel->setText(QString::number(obj["humi"].toDouble(), 'f', 1));
    if (obj.contains("mq2"))  smokeValueLabel->setText(QString::number(obj["mq2"].toInt()));

    if (obj.contains("light")) {
        lightValueLabel->setText(QString::number(obj["light"].toInt()));
    }
    if (obj.contains("led_brightness")) {
            ledBrightnessLabel->setText(QString("💡 LED状态: %1%").arg(obj["led_brightness"].toInt()));
    }

    /* 同步锁钮状态 + 自动模式下滑块跟随实际亮度 */
    if (obj.contains("led_mode")) {
        bool isManual = (obj["led_mode"].toInt() == 0);
        lockBtn->blockSignals(true);
        lockBtn->setChecked(isManual);
        lockBtn->blockSignals(false);
        if (!isManual && obj.contains("led_brightness")) {
            ledSlider->blockSignals(true);
            ledSlider->setValue(obj["led_brightness"].toInt());
            ledSlider->blockSignals(false);
        }
    }

           // 2. ======== 核心五点滤波图表追加算法 ========
    if (obj.contains("temp") && obj.contains("humi") && obj.contains("mq2")) {
        tempQueue.enqueue(obj["temp"].toDouble());
        humiQueue.enqueue(obj["humi"].toDouble());
        smokeQueue.enqueue(obj["mq2"].toDouble());

        if (tempQueue.size() >= 5) {
            double tempSum = 0, humiSum = 0, smokeSum = 0;
            int actualSize = tempQueue.size();
            while (!tempQueue.isEmpty()) {
                tempSum += tempQueue.dequeue();
                humiSum += humiQueue.dequeue();
                smokeSum += smokeQueue.dequeue();
            }
            double tempAvg = tempSum / actualSize;
            double humiAvg = humiSum / actualSize;
            double smokeAvg = (smokeSum / actualSize) / 10.0; //     double smokeAvg = (smokeSum / actualSize) / 10.0; // 烟雾值缩小10倍

            if (!m_isHistoryMode && tempSeries && humiSeries && smokeSeries) {
                tempSeries->append(m_chartXCount, tempAvg);
                humiSeries->append(m_chartXCount, humiAvg);
                smokeSeries->append(m_chartXCount, smokeAvg);
                m_chartXCount++;

                if (m_chartXCount > 30 && axisX) {
                    axisX->setRange(m_chartXCount - 30, m_chartXCount);
                }
            }
        }
    }

               // 2.5 RFID 卡号处理
    // 3. 刷新安防工业灯图形颜色与文本提示（完成汉化）
    QLabel* pirLbl = qvariant_cast<QLabel*>(this->property("pir_lbl"));
    QLabel* flmLbl = qvariant_cast<QLabel*>(this->property("flm_lbl"));
    QLabel* srvLbl = qvariant_cast<QLabel*>(this->property("srv_lbl"));

    QString safeStyle = "background:#052E16; border:1.5px solid #22c55e; border-radius:6px; font-size:12px; font-weight:bold; color:#bbf7d0;";

    QString dangerStyle = 
        "background:#450a0a; border:1.5px solid #ef4444; border-radius:6px; font-size:12px; font-weight:bold; color:#fecaca;";

    if (obj.contains("pir") && pirLbl) {
        int pir = obj["pir"].toInt();
        int servo = obj.contains("servo") ? obj["servo"].toInt() : (m_doorOpen ? 1 : 0);
        int pirAlert = (pir == 1 && servo == 0);  /* 门开着时 PIR 不报警 */
        pirLbl->setStyleSheet(pirAlert ? dangerStyle : safeStyle);
        pirLbl->setText(pirAlert ? "红外安防\n状态：触发" : "红外安防\n状态：正常");
    }
    if (obj.contains("flame") && flmLbl) {
        int flame = obj["flame"].toInt();
        flmLbl->setStyleSheet(flame == 1 ? dangerStyle : safeStyle);
        flmLbl->setText(flame == 1 ? "火焰高危\n状态：触发" : "火焰高危\n状态：正常");
    }
    if (obj.contains("servo") && srvLbl) {
        int servo = obj["servo"].toInt();
        /* 非对称消抖: 开门立刻显示, 关门需连续两帧确认(防偶发错帧闪烁) */
        if (servo == 1 || m_lastServoRaw == 0) {
            srvLbl->setStyleSheet(servo == 1 ? safeStyle : dangerStyle);
            srvLbl->setText(servo == 1 ? "🔓 仓门已开" : "🔒 仓门已关");
            m_doorOpen = (servo == 1);
            doorBtn->setText(m_doorOpen ? "关门" : "开门");
            doorBtn->setStyleSheet(m_doorOpen ? "QPushButton { background-color: #16a34a; color: #fff; border-radius: 6px; font-weight: bold; } QPushButton:hover { background-color: #15803d; }" : "QPushButton { background-color: #dc2626; color: #fff; border-radius: 6px; font-weight: bold; } QPushButton:hover { background-color: #b91c1c; }");
        }
        m_lastServoRaw = servo;
    }

    // sensor_logs 累加器: 每条MQTT数据都喂入，10秒后取均值写入库
    double t  = obj.contains("temp")   ? obj["temp"].toDouble() : 0;
    double h  = obj.contains("humi")   ? obj["humi"].toDouble() : 0;
    int    m  = obj.contains("mq2")    ? obj["mq2"].toInt()    : 0;
    int    l  = obj.contains("light")  ? obj["light"].toInt()  : 0;
    int    f  = obj.contains("fan")    ? obj["fan"].toInt()    : 0;
    int    lb = obj.contains("led_brightness") ? obj["led_brightness"].toInt() : 0;
    int    lm = obj.contains("led_mode")       ? obj["led_mode"].toInt()       : 1;
    int    p  = obj.contains("pir")    ? obj["pir"].toInt()    : 0;
    int    fl = obj.contains("flame")  ? obj["flame"].toInt()  : 0;
    int    s  = obj.contains("servo")  ? obj["servo"].toInt()  : 0;
    m_accum.feed(t, h, m, l, f, lb, lm, p, fl, s);
}

// =========================================================================
// 💾 sensor_logs 10秒聚合写入数据库
// =========================================================================
void MainWindow::flushSensorLogs()
{
    if (m_accum.count == 0) return;

    QSqlQuery q;
    q.prepare("INSERT INTO sensor_logs "
              "(temp, humidity, mq2, light, fan_speed, led_bright, led_mode, pir_alert, flame_alert, servo_state) "
              "VALUES (:t, :h, :m, :l, :f, :lb, :lm, :p, :fl, :s)");
    q.bindValue(":t",  m_accum.tempSum  / m_accum.count);
    q.bindValue(":h",  m_accum.humiSum  / m_accum.count);
    q.bindValue(":m",  m_accum.mq2Sum   / m_accum.count);
    q.bindValue(":l",  m_accum.lightSum / m_accum.count);
    q.bindValue(":f",  m_accum.fanSum   / m_accum.count);
    q.bindValue(":lb", m_accum.ledSum   / m_accum.count);
    q.bindValue(":lm", m_accum.ledMode);
    q.bindValue(":p",  m_accum.pirAlert);
    q.bindValue(":fl", m_accum.flameAlert);
    q.bindValue(":s",  m_accum.servoState);

    if (q.exec()) {
        qDebug() << "[DB] sensor_logs 写入成功, 样本数:" << m_accum.count;
    } else {
        qDebug() << "[DB] sensor_logs 写入失败:" << q.lastError().text();
    }
    m_accum.reset();
}

// =========================================================================
// 📊 历史回溯: 从 sensor_logs 加载数据到图表
// =========================================================================
void MainWindow::loadHistory(int hours)
{
    m_isHistoryMode = true;
    m_lastHistoryHours = hours;
    m_histRefreshTimer->start(10000); // 每10秒自动刷新

    QSqlQuery q;
    q.prepare("SELECT temp, humidity, mq2, light, fan_speed, led_bright, recorded_at "
              "FROM sensor_logs WHERE recorded_at >= datetime('now', :dur) "
              "ORDER BY recorded_at DESC");
    q.bindValue(":dur", QString("-%1 hours").arg(hours));
    q.exec();

    // 先全部读到内存，方便算统计
    struct Row { double t,h; int m,l,f,lb; QString ts; };
    QList<Row> rows;
    double tSum = 0, hSum = 0, tMax = -999, tMin = 999, hMax = -999, hMin = 999;
    while (q.next()) {
        Row r;
        r.t  = q.value(0).toDouble(); r.h  = q.value(1).toDouble();
        r.m  = q.value(2).toInt();    r.l  = q.value(3).toInt();
        r.f  = q.value(4).toInt();    r.lb = q.value(5).toInt();
        r.ts = q.value(6).toString();
        rows.append(r);
        tSum += r.t; hSum += r.h;
        if (r.t > tMax) tMax = r.t; if (r.t < tMin) tMin = r.t;
        if (r.h > hMax) hMax = r.h; if (r.h < hMin) hMin = r.h;
    }
    int n = rows.size();
    if (n == 0) { tMax=tMin=hMax=hMin=0; }

    // 摘要卡片
    m_histAvgTemp->setText(QString("%1℃").arg(tSum / n, 0, 'f', 1));
    m_histMaxTemp->setText(QString("%1℃").arg(tMax, 0, 'f', 1));
    m_histMinTemp->setText(QString("%1℃").arg(tMin, 0, 'f', 1));
    m_histAvgHumi->setText(QString("%1%").arg(hSum / n, 0, 'f', 1));
    m_histMaxHumi->setText(QString("%1%").arg(hMax, 0, 'f', 1));
    m_histMinHumi->setText(QString("%1%").arg(hMin, 0, 'f', 1));
    m_histSampleCount->setText(QString("%1条").arg(n));

    // 填充表格（最新在上）
    m_histTable->setRowCount(n);
    for (int i = 0; i < n; i++) {
        const Row &r = rows[i];
        // 时间只保留时分秒
        QString timeStr = r.ts.length() >= 19 ? r.ts.mid(11, 8) : r.ts;
        m_histTable->setItem(i, 0, new QTableWidgetItem(timeStr));
        m_histTable->setItem(i, 1, new QTableWidgetItem(QString::number(r.t,  'f', 1)));
        m_histTable->setItem(i, 2, new QTableWidgetItem(QString::number(r.h,  'f', 1)));
        m_histTable->setItem(i, 3, new QTableWidgetItem(QString::number(r.m)));
        m_histTable->setItem(i, 4, new QTableWidgetItem(QString::number(r.l)));
        m_histTable->setItem(i, 5, new QTableWidgetItem(QString::number(r.f)));
        m_histTable->setItem(i, 6, new QTableWidgetItem(QString::number(r.lb)));
    }

    m_chartStack->setCurrentIndex(1); // 切换到历史面板
    qDebug() << "[历史] 加载了" << n << "条记录, 时间范围:" << hours << "小时";
}

void MainWindow::switchToRealtime()
{
    m_isHistoryMode = false;
    m_historyBar->setVisible(false);
    m_histRefreshTimer->stop();       // 停止自动刷新
    m_chartStack->setCurrentIndex(0); // 切换回实时图表
}

// =========================================================================
// 🔐     // 🔐 权限与 MQTT 状态刷新控制逻辑
// =========================================================================
void MainWindow::applyPermissions()
{
    fanBtn->setEnabled(true);
    lockBtn->setEnabled(true);
    fanSlider->setEnabled(true);
    ledSlider->setEnabled(true);

    if (m_currentUserRole == "admin") {
        manageUsersBtn->setVisible(true);
        cardMgrBtn->setVisible(true);
        this->setWindowTitle("仓瞳物联网控制总线 - [系统终极管理员控制台]");
    } else {
        manageUsersBtn->setVisible(false);
        cardMgrBtn->setVisible(false);
        this->setWindowTitle("仓瞳物联网控制总线 - [值班人员远程控制台]");
    }
}

void MainWindow::onMqttConnected()
{
    updateMqttStatus(true);
    qDebug() << "物联总线已成功激活！正在监听远程数据中心...";
    m_mqttClient->subscribeToTopic("wms/warehouse1/#");
}

void MainWindow::updateMqttStatus(bool connected)
{
    if (connected) {
        mqttStatusLabel->setText("●  物联网络: 在线  |  EMQX Edge");
        mqttStatusLabel->setStyleSheet(
            "color:#10B981;"
            "font-size:13px;"
            "font-weight:600;"
            );
    } else {
        mqttStatusLabel->setText("● 物联网络: 离线断开");
        mqttStatusLabel->setStyleSheet(
            "color:#EF4444;"
            "font-size:13px;"
            "font-weight:600;"
            );
    }
}


void MainWindow::onMqttDisconnected()
{
    updateMqttStatus(false);
}

void MainWindow::onLogoutClicked()
{
    this->hide();
    QString role = showLoginDialog();
    if (role.isEmpty()) {
        qApp->quit();
    } else {
        m_currentUserRole = role;
        applyPermissions();
        this->show();
    }
}

void MainWindow::onManageUsersClicked()
{
    QDialog mgrDlg(this);
    mgrDlg.setWindowTitle("账号凭证安全中心");
    mgrDlg.resize(450, 320);
    mgrDlg.setStyleSheet("QDialog { background-color: #0B1120; } QLabel { color: #94a3b8; }");

    QVBoxLayout *layout = new QVBoxLayout(&mgrDlg);
    QListWidget *userList = new QListWidget(&mgrDlg);
    userList->setStyleSheet("background: #111827; color: #fff; border-radius: 6px; border:1px solid #1E293B;");
    layout->addWidget(new QLabel("当前系统内注册的合法凭证列表:", &mgrDlg));
    layout->addWidget(userList);

    auto refreshUserList = [userList]() {
        userList->clear();
        QSqlQuery q("SELECT username, role FROM users");
        while (q.next()) {
            userList->addItem(QString("👤 账号: %1  [%2]").arg(q.value(0).toString()).arg(q.value(1).toString()));
        }
    };
    refreshUserList();

    QHBoxLayout *form = new QHBoxLayout();
    QLineEdit *usernameEdit = new QLineEdit(&mgrDlg); usernameEdit->setPlaceholderText("新账户");
    QLineEdit *passwordEdit = new QLineEdit(&mgrDlg); passwordEdit->setPlaceholderText("新密码");
    QComboBox *roleBox = new QComboBox(&mgrDlg); roleBox->addItems({"user", "admin"});
    usernameEdit->setStyleSheet("background: #111827; color: #fff; padding: 4px; border:1px solid #1E293B; border-radius:4px;");
    passwordEdit->setStyleSheet("background: #111827; color: #fff; padding: 4px; border:1px solid #1E293B; border-radius:4px;");
    form->addWidget(usernameEdit); form->addWidget(passwordEdit); form->addWidget(roleBox);
    layout->addLayout(form);

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("➕ 添加或授权", &mgrDlg);
    QPushButton *delBtn = new QPushButton("🗑️ 彻底销毁凭证", &mgrDlg);
    addBtn->setStyleSheet("background: #2563EB; color: #fff; padding: 6px; border-radius: 4px;");
    delBtn->setStyleSheet("background: #EF4444; color: #fff; padding: 6px; border-radius: 4px;");
    btns->addWidget(addBtn); btns->addWidget(delBtn);
    layout->addLayout(btns);

    connect(addBtn, &QPushButton::clicked, this, [&mgrDlg, usernameEdit, passwordEdit, roleBox, refreshUserList, this]() {
        QString name = usernameEdit->text().trimmed();
        QString pass = passwordEdit->text().trimmed();
        if (name.isEmpty() || pass.isEmpty()) return;

        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO users (username, password, role) VALUES (:name, :pass, :role)");
        query.bindValue(":name", name);
        QByteArray hash = QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256).toHex();
        query.bindValue(":pass", QString::fromUtf8(hash));
        query.bindValue(":role", roleBox->currentText());
        if (query.exec()) {
            QMessageBox::information(&mgrDlg, "成功", "安全账号数据库已完成同步！");
            usernameEdit->clear(); passwordEdit->clear();
            refreshUserList();
        }
    });

    connect(delBtn, &QPushButton::clicked, this, [&mgrDlg, userList, usernameEdit, passwordEdit, refreshUserList, this]() {
        QListWidgetItem *item = userList->currentItem();
        if (!item) return;
        QString text = item->text();
        QString name = text.split(" ")[2];
        if (name == "admin") {
            QMessageBox::warning(&mgrDlg, "拒绝访问", "根节点的出厂默认管理员无法被销毁！");
            return;
        }
        QSqlQuery query;
        query.prepare("DELETE FROM users WHERE username = :name");
        query.bindValue(":name", name);
        if (query.exec()) {
            QMessageBox::information(&mgrDlg, "成功", "该账号已被彻底从本地集群移除！");
            usernameEdit->clear(); passwordEdit->clear();
            refreshUserList();
        }
    });

    mgrDlg.exec();
}

QString MainWindow::showLoginDialog()
{
    QDialog loginDlg;
    loginDlg.setWindowTitle("仓瞳边缘总线鉴权安全门禁");
    loginDlg.setFixedSize(380, 240);
    loginDlg.setStyleSheet("QDialog { background-color: #0B1120; } QLabel { color: #94a3b8; font-size:12px; }");

    QVBoxLayout *layout = new QVBoxLayout(&loginDlg);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(12);

    QLabel *title = new QLabel("🔀 C-EYE BUS ACCESS SECURITY", &loginDlg);
    title->setStyleSheet("font-size:14px; font-weight:bold; color:#3b82f6; font-family:'Consolas';");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLineEdit *userEdit = new QLineEdit(&loginDlg); userEdit->setPlaceholderText("输入操作员账号(admin/user1)");
    QLineEdit *passEdit = new QLineEdit(&loginDlg); passEdit->setPlaceholderText("输入安全通行密码");
    passEdit->setEchoMode(QLineEdit::Password);

    QString editStyle = "QLineEdit { background-color: #111827; border: 1px solid #1E293B; border-radius: 6px; color: #fff; padding: 8px; font-size:13px; }";
    userEdit->setStyleSheet(editStyle); passEdit->setStyleSheet(editStyle);
    layout->addWidget(userEdit); layout->addWidget(passEdit);

    QPushButton *loginBtn = new QPushButton("安全登录", &loginDlg);
    loginBtn->setStyleSheet("QPushButton { background-color: #2563EB; color: #fff; border-radius: 6px; padding: 10px; font-weight:bold; font-size:13px; }"
                            "QPushButton:hover { background-color: #1d4ed8; }");
    layout->addWidget(loginBtn);

    QString loginRole = "";
    connect(loginBtn, &QPushButton::clicked, &loginDlg, [&]() {
        QString inputUser = userEdit->text().trimmed();
        QString inputPass = passEdit->text();

        if (inputUser.isEmpty() || inputPass.isEmpty()) {
            QMessageBox::warning(&loginDlg, "提示", "字段不可为空，请输入有效数据！");
            return;
        }

        QSqlQuery query;
        query.prepare("SELECT role FROM users WHERE username = :user AND password = :pass");
        query.bindValue(":user", inputUser);
        QByteArray hash = QCryptographicHash::hash(inputPass.toUtf8(), QCryptographicHash::Sha256).toHex();
        query.bindValue(":pass", QString::fromUtf8(hash));

        if (query.exec() && query.next()) {
            loginRole = query.value(0).toString();
            loginDlg.accept();
        } else {
            QMessageBox::warning(&loginDlg, "拒绝访问", "认证未通过：账户凭证或密码不匹配！");
        }
    });

    if (loginDlg.exec() == QDialog::Accepted) {
        return loginRole;
    }
    return "";
}

void MainWindow::initLocalDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("stwhouse_user.db");
    if (!db.open()) {
        qDebug() << "本地持久化账户断开链接:" << db.lastError().text();
        return;
    }
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT, role TEXT)");
    // 迁移旧明文密码 -> SHA-256 哈希
    q.exec("UPDATE users SET password = '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92' WHERE password = '123456'");
    q.exec("INSERT OR IGNORE INTO users (username, password, role) VALUES ('admin', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'admin')");
    q.exec("CREATE TABLE IF NOT EXISTS cards (uid TEXT PRIMARY KEY, label TEXT, authorized INTEGER DEFAULT 0, created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");
    q.exec("INSERT OR IGNORE INTO cards (uid, authorized) VALUES ('81F42C07', 1)");
    q.exec("INSERT OR IGNORE INTO users (username, password, role) VALUES ('user1', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'user')");

    // ======== sensor_logs 历史数据表 ========
    q.exec("PRAGMA journal_mode=WAL;");
    q.exec("CREATE TABLE IF NOT EXISTS sensor_logs ("
           "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
           "temp        REAL,"
           "humidity    REAL,"
           "mq2         INTEGER,"
           "light       INTEGER,"
           "fan_speed   INTEGER,"
           "led_bright  INTEGER,"
           "led_mode    INTEGER,"
           "pir_alert   INTEGER,"
           "flame_alert INTEGER,"
           "servo_state INTEGER,"
           "recorded_at DATETIME DEFAULT CURRENT_TIMESTAMP"
           ");");
    q.exec("DELETE FROM sensor_logs WHERE recorded_at < datetime('now', '-7 days');");
    qDebug() << "[DB] sensor_logs 表已就绪，WAL模式，7天自动清理";
}

MainWindow::~MainWindow() {}
