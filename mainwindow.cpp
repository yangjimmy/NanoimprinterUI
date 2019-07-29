#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QString>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->txtData->setPlainText("Welcome\n");


    // set up file
    log = new QFile("log.txt");
    log->open(QIODevice::WriteOnly);

    QTextStream data(log);
    data << "-- begin run --" << endl;

    // set up serial
    arduino_is_available = false;
    arduino_port_name = "";
    arduino = new QSerialPort;

    setupSPI();
    paused = true;

    // set up time ticker
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");

    // set up pressure graph ///////////////////////////////////////////////////////////////////////////////////////
    ui->plot_pres->addGraph(); // blue line
    ui->plot_pres->graph(0)->setPen(QPen(QColor(40, 110, 255)));

    ui->plot_pres->xAxis->setTicker(timeTicker);
    ui->plot_pres->axisRect()->setupFullAxesBox();
    ui->plot_pres->yAxis->setRange(-1.2, 1.2);

    ui->plot_pres->xAxis->setLabel("Time elapsed (s)");
    ui->plot_pres->yAxis->setLabel("Pressure (Bar)");

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->plot_pres->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot_pres->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot_pres->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot_pres->yAxis2, SLOT(setRange(QCPRange)));

    // set up temperature graph ///////////////////////////////////////////////////////////////////////////////////////
    ui->plot_temp->addGraph(); // blue line
    ui->plot_temp->graph(0)->setPen(QPen(QColor(40, 110, 255)));

    ui->plot_temp->xAxis->setTicker(timeTicker);
    ui->plot_temp->axisRect()->setupFullAxesBox();
    ui->plot_temp->yAxis->setRange(-1.2, 1.2);

    ui->plot_temp->xAxis->setLabel("Time elapsed (s)");
    ui->plot_temp->yAxis->setLabel("Temperature (C)");

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->plot_temp->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot_temp->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plot_temp->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot_temp->yAxis2, SLOT(setRange(QCPRange)));

    // setup a timer that repeatedly calls MainWindow::realtimeDataSlot: //////////////////////////////////////////////
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    dataTimer.start(0); // Interval 0 means to refresh as fast as possible
}

void MainWindow::setupSPI(){

    // checks if arduino is connected
    // checks if port is available

    foreach (const QSerialPortInfo &serialportinfo, QSerialPortInfo::availablePorts()){
        if(serialportinfo.hasVendorIdentifier() && serialportinfo.hasProductIdentifier()){
            if(serialportinfo.vendorIdentifier() == arduino_mega_vendor_id && serialportinfo.productIdentifier() == arduino_mega_product_id) {
                arduino_port_name = serialportinfo.portName();
                arduino_is_available = true;
            }
        }
    }

    if (arduino_is_available) {
        // set serial port name
        arduino->setPortName(arduino_port_name);
        arduino->open(QSerialPort::ReadWrite);
        arduino->setBaudRate(QSerialPort::Baud9600);
        arduino->setDataBits(QSerialPort::Data8);
        arduino->setParity(QSerialPort::NoParity);
        arduino->setStopBits(QSerialPort::OneStop);
        arduino->setFlowControl(QSerialPort::NoFlowControl);
    }
}

void MainWindow::realtimeDataSlot()
{
    QTextStream data(log);

    if (!paused){
        static QTime time(QTime::currentTime());

        // add new data to plot
        double tick = time.elapsed()/1000.0; // time elapsed since start of demo, in seconds
        static double lastTick = 0;
        if (tick-lastTick > 0.1) {
            QString readCommandPressure = "#RPRS\n";
            send(readCommandPressure);
            /*
            QThread::msleep(50);
            while (!(arduino->waitForReadyRead())){

            }
            */
            QByteArray rawPressureData = arduino->readLine();
            double currPressure = rawPressureData.trimmed().toDouble();

            if (currPressure > (double)35.0){
                //temporary fix for random spikes
                currPressure = 0;
            }
            arduino->clear();

            QString readCommandTemp = "#RTMP\n";
            send(readCommandTemp);
            /*
            QThread::msleep(50);
            while (!(arduino->waitForReadyRead())){

            }
            */
            QByteArray rawTempData = arduino->readLine();
            double currTemperature = rawTempData.trimmed().toDouble();
            arduino->clear();

            // add data to lines:
            data << QString::number(tick) << endl;

            ui->plot_pres->graph(0)->addData(tick, currPressure);

            ui->plot_temp->graph(0)->addData(tick, currTemperature);

            // rescale value (vertical) axis to fit the current data:
            ui->plot_pres->graph(0)->rescaleValueAxis();
            ui->plot_temp->graph(0)->rescaleValueAxis();
            lastTick = tick;

            // write to output in mainwindow data tab
            ui->txtData->moveCursor(QTextCursor::End);

            ui->txtData->insertPlainText("Current Time: ");
            ui->txtData->insertPlainText(QString::number(tick));
            ui->txtData->insertPlainText("\n");

            ui->txtData->insertPlainText("Current Pressure: ");
            ui->txtData->insertPlainText(QString::number(currPressure));
            ui->txtData->insertPlainText("\n");

            ui->txtData->insertPlainText("Current Temperature: ");
            ui->txtData->insertPlainText(QString::number(currTemperature));
            ui->txtData->insertPlainText("\n");

            // write to log file

            //data << "Current Time: " << endl;
            data << QString::number(tick);
            data << ",";

            //data << "Current Pressure: " << endl;
            data << QString::number(currPressure);
            data << ",";

            //data << "Current Temperature: " << endl;
            data << QString::number(currTemperature) << endl;
        }
        /*
        if (tick-lastTick > 1 && arduino->waitForReadyRead()) // refresh every 1s and see if there is data to be read
        {
            QByteArray rawData;

            qDebug() << "bytes available:" << arduino->bytesAvailable();

            // read in data
            rawData = arduino->readLine();
            qDebug() << "length of raw data" << rawData.length();
            //arduino->clear();

            // print raw data to debug console
            qDebug() << "raw data:" << rawData;
            qDebug() << "trimmed raw data:" << rawData.trimmed();
            //qDebug() << "type of trimmed data:" << typeid(rawData.trimmed()).name();

            // extract contents from QByteArray and convert the values to floating point numbers
            std::string newData = rawData.trimmed().toStdString();
            QString qstr = QString::fromStdString(newData);
            double currPressure = (qstr.section(qstr.indexOf("P"), qstr.indexOf("T")-1)).toDouble();
            double currTemperature = (qstr.section(qstr.indexOf("T"), qstr.length()-1)).toDouble();
            qDebug() << "integer" << qstr;
            qDebug() << "pressure" << currPressure;
            qDebug() << "temperature" << currTemperature;

            // add data to lines:
            ui->plot_pres->graph(0)->addData(tick, currPressure);
            ui->plot_temp->graph(0)->addData(tick, currTemperature);
            // rescale value (vertical) axis to fit the current data:
            ui->plot_pres->graph(0)->rescaleValueAxis();
            ui->plot_temp->graph(0)->rescaleValueAxis();
            lastTick = tick;
        }
        */
        ui->plot_pres->xAxis->setRange(0, -time.elapsed()/1000.0, Qt::AlignRight);
        ui->plot_pres->replot();
        ui->plot_temp->xAxis->setRange(0, -time.elapsed()/1000.0, Qt::AlignRight);
        ui->plot_temp->replot();
    }
}

MainWindow::~MainWindow()
{
    // closes port when window closes
    if (arduino->isOpen()){
        arduino->close();
    }
    delete ui;
}

void MainWindow::on_btn_run_clicked()
{
    qDebug() << "run clicked";
    paused = false;
    // read in values from spinboxes
    pressureSetPoint=ui->sb_pres->value();
    pressureKd = ui->sb_kd_pres->value();
    pressureKi = ui->sb_ki_pres->value();
    pressureKp = ui->sb_kp_pres->value();

    tempSetPoint=ui->sb_temp->value();
    tempKd=ui->sb_kd_temp->value();
    tempKi=ui->sb_ki_temp->value();
    tempKp=ui->sb_kp_temp->value();
    heatTime=ui->sb_heat_time->value();

    // format values
    QString pressureCommand="#SPRS";
    pressureCommand.append("S");
    pressureCommand.append(QString::number(pressureSetPoint));
    pressureCommand.append("P");
    pressureCommand.append(QString::number(pressureKp));
    pressureCommand.append("I");
    pressureCommand.append(QString::number(pressureKi));
    pressureCommand.append("D");
    pressureCommand.append(QString::number(pressureKd));
    pressureCommand.append("\n"); // end of command

    QString tempCommand="#STMP";
    tempCommand.append("S");
    tempCommand.append(QString::number(tempSetPoint));
    tempCommand.append("P");
    tempCommand.append(QString::number(tempKp));
    tempCommand.append("I");
    tempCommand.append(QString::number(tempKi));
    tempCommand.append("D");
    tempCommand.append(QString::number(tempKd));
    tempCommand.append("T");
    tempCommand.append(QString::number(heatTime));
    tempCommand.append("\n");

    // send commands to Arduino
    send(pressureCommand);
    send(tempCommand);
}

// helper function to send command
void MainWindow::send(QString command){
    if(arduino->isWritable()){
        arduino->write(command.toStdString().c_str()); // converts first to c++ string (std library) then converts to a c-type "string" (aka pointer)
    } else {
        qDebug() << "Unable to write to serial"; // error check
    }
}
