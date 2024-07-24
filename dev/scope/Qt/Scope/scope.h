#ifndef SCOPE_H
#define SCOPE_H

#include <QMainWindow>
#include <QTimer>
#include <QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
//#include "pcomm.h"
#include "qplotbox.h"




QT_BEGIN_NAMESPACE
namespace Ui { class Scope; }
QT_END_NAMESPACE

class Scope : public QMainWindow
{
    Q_OBJECT

public:
    Scope(QWidget *parent = nullptr);
    ~Scope();


private slots:
    void on_singleButton_pressed();
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

    void on_HZoomSpinBox_valueChanged(double arg1);

    void on_VZoomSpinBox_valueChanged(double arg1);

private:
    Ui::Scope *ui;
    QPlotBox *plotbox;
    // Timer
    QTimer *timer1ms;
    uint32_t plot_repaint_time;

    void onTimer1ms();
};
#endif // SCOPE_H
