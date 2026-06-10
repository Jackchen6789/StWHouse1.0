#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSqlDatabase>
#include <QQueue>
#include <QTimer>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include "simplemqttclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString showLoginDialog();

private slots:
    void handleMqttMessage(const QString &topic, const QByteArray &payload);
    void onMqttConnected();
    void onMqttDisconnected();
    void onLogoutClicked();
    void onManageUsersClicked();

private:
    Ui::MainWindow *ui;
    SimpleMqttClient *m_mqttClient;

    // ?????????
    QLabel *tempValueLabel;
    QLabel *humiValueLabel;
    QLabel *smokeValueLabel;
    QLabel *lightValueLabel;
    QLabel *ledBrightnessLabel;  // LED??????
    QLabel *mqttStatusLabel;  // MQTT ??????

    // ???????
    QPushButton *fanBtn;
    QPushButton *lockBtn;
    QPushButton *doorBtn;     // ??????
    bool m_doorOpen;
    int m_lastServoRaw = 0;
    QSlider *fanSlider;
    QSlider *ledSlider;

    // ??????
    QPushButton *logoutBtn;
    QPushButton *manageUsersBtn;
    QPushButton *cardMgrBtn;
    QString m_currentUserRole;

    // ??????
    QChartView  *chartView;
    QLineSeries *tempSeries;
    QLineSeries *humiSeries;
    QLineSeries *smokeSeries;
    QValueAxis  *axisX;
    QValueAxis  *axisY;
    int          m_chartXCount;

    // 100????????
    QQueue<double> tempQueue;
    QQueue<double> humiQueue;
    QQueue<double> smokeQueue;

    // 历史回溯
    QPushButton *m_btnRealtime;
    QPushButton *m_btnHistory;
    QWidget     *m_historyBar;
    QPushButton *m_btnH1h, *m_btnH6h, *m_btnH24h;
    bool         m_isHistoryMode;

    // 历史面板
    QStackedWidget *m_chartStack;
    QWidget        *m_historyPanel;
    QLabel         *m_histAvgTemp, *m_histMaxTemp, *m_histMinTemp;
    QLabel         *m_histAvgHumi, *m_histMaxHumi, *m_histMinHumi;
    QLabel         *m_histSampleCount;
    QTableWidget   *m_histTable;
    int             m_lastHistoryHours;
    QTimer         *m_histRefreshTimer;

    // ?????

    // sensor_logs 10秒聚合写入
    struct SensorAccumulator {
        int count = 0;
        double tempSum = 0, humiSum = 0;
        int mq2Sum = 0, lightSum = 0, fanSum = 0, ledSum = 0;
        int ledMode = 1, pirAlert = 0, flameAlert = 0, servoState = 0;

        void feed(double t, double h, int m, int l, int f, int lb, int lm, int p, int fl, int s) {
            count++; tempSum += t; humiSum += h;
            mq2Sum += m; lightSum += l; fanSum += f; ledSum += lb;
            ledMode = lm;
            if (p) pirAlert = 1; if (fl) flameAlert = 1;
            servoState = s;
        }
        void reset() {
            count = 0; tempSum = humiSum = 0;
            mq2Sum = lightSum = fanSum = ledSum = 0;
            pirAlert = flameAlert = 0;
        }
    };
    SensorAccumulator m_accum;
    QTimer *m_logTimer;

    void initLocalDatabase();
    void applyPermissions();
    void setupChart();
    void updateMqttStatus(bool connected);
    void switchToRealtime();
    void loadHistory(int hours);

private slots:
    void flushSensorLogs();
};
#endif // MAINWINDOW_H
