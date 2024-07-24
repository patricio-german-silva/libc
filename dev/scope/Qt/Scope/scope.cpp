#include "scope.h"
#include "ui_scope.h"

Scope::Scope(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Scope)
{
    timer1ms = new QTimer();
    timer1ms->setInterval(1);
    timer1ms->start();
    plot_repaint_time = 10;
    connect(timer1ms, &QTimer::timeout, this, &Scope::onTimer1ms);

    ui->setupUi(this);
    plotbox = new QPlotBox(0, 0, ui->framePlot);
    plotbox->plotSetStyle("gray", 3, "Arial", 8, "White", "v", true);
    plotbox->plotGrid();
    plotbox->update();
}

Scope::~Scope()
{
    delete timer1ms;
    delete plotbox;
    delete ui;
}


void Scope::onTimer1ms()
{
    static uint8_t drawCount = 0;
    static uint32_t paintbox_busy_count = 0;
    static bool paintbox_busy = false;

    // Realizo los graficos
    if(drawCount++ > plot_repaint_time){
        if(!paintbox_busy){
            paintbox_busy = true;
            plotbox->erasePlotBox();
            plotbox->plotGrid();
            plotbox->update();
            drawCount = 0;
            paintbox_busy = false;
        }else{
            paintbox_busy_count++;
        }
    }
}

void Scope::paintEvent(QPaintEvent *event)
{
    static bool first = false;
    if(!first){
        plotbox->resize(ui->framePlot->size());
        first = true;
    }
}

void Scope::resizeEvent(QResizeEvent *event)
{
    plotbox->resize(ui->framePlot->size());
}
void Scope::on_singleButton_pressed()
{

}


void Scope::on_HZoomSpinBox_valueChanged(double arg1)
{
    plotbox->plotSetZoom(ui->HZoomSpinBox->value(), ui->VZoomSpinBox->value());
}


void Scope::on_VZoomSpinBox_valueChanged(double arg1)
{
    plotbox->plotSetZoom(ui->HZoomSpinBox->value(), ui->VZoomSpinBox->value());
}

