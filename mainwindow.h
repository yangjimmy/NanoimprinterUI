#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSerialPort>
#include <QString>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    //plotting
    void clearData();
    //SPI
    void setupSPI();
    double serialReceived();

private slots:
    void on_btn_clear_clicked();
    void realtimeDataSlot();
    void on_btn_pause_clicked();
    void send(QString command);
    void on_btn_run_clicked();

private:
    Ui::MainWindow *ui;
    //plotting
    QTimer dataTimer;
    double newData;
    bool paused;
    double pressureSetPoint;
    double pressureKi;
    double pressureKp;
    double pressureKd;
    double tempSetPoint;
    double tempKi;
    double tempKp;
    double tempKd;
    //SPI
    QSerialPort *arduino;
    //static const quint16 arduino_mega_vendor_id = 9025; // fixed for specific product (this case, arduino mega)
    static const quint16 arduino_mega_vendor_id = 10755;
    static const quint16 arduino_mega_product_id = 66; // fixed for specific product (this case, arduino mega)
    QString arduino_port_name;
    bool arduino_is_available = true;
};

#endif // MAINWINDOW_H
