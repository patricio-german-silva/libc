#include "qplotbox.h"

QPlotBox::QPlotBox(int aWidth, int aHeight, QWidget *parent) : QWidget(parent)
{
    if(parent){
        aWidth = parent->width();
        aHeight = parent->height();
    }

    pixelCanvas = new QPixmap(aWidth, aHeight);
    this->resize(aWidth, aHeight);
    pixelCanvas->fill(Qt::black);

    plot_config.plot_grid_y_count = 10;
    plot_config.plot_grid_y_starts = 0;
    plot_config.plot_grid_y_ends = 10;
    plot_config.plot_grid_y_unit = "";
    plot_config.plot_grid_x_count = 10;
    plot_config.plot_grid_x_starts = 0;
    plot_config.plot_grid_x_ends = 10;
    plot_config.plot_grid_x_is_us = false;
    plot_config.plot_grid_line_color = "gray";
    plot_config.plot_grid_line_style = 1;
    plot_config.plot_grid_font_name = "Arial";
    plot_config.plot_grid_font_size = 8;
    plot_config.plot_back_color = "Black";
    plot_config.plot_x_zoom = 1;
    plot_config.plot_y_zoom = 1;
    plot_config.show_x_cursors = false;
    plot_config.show_y_cursors = false;
}

//------------------------------------------------------------------------------
void QPlotBox::paintEvent(QPaintEvent * /* event */){
    QPainter Canvas(this);

    Canvas.drawPixmap(0,0,*pixelCanvas);

}
//------------------------------------------------------------------------------
void QPlotBox::resizeEvent(QResizeEvent */* event */){
    QPixmap aux(this->width(), this->height());

    aux.copy(pixelCanvas->rect());
    pixelCanvas->scaled(aux.width(), aux.height());
    pixelCanvas->swap(aux);
}
//------------------------------------------------------------------------------
void QPlotBox::setWidth(int aWidth){
    pixelCanvas->size().setWidth(aWidth);
    this->resize(aWidth, this->height());
}
//------------------------------------------------------------------------------
void QPlotBox::setHeight(int aHeight){
    pixelCanvas->size().setWidth(aHeight);
    this->resize(this->width(), aHeight);
}


//------------------------------------------------------------------------------
// Paint functions
//------------------------------------------------------------------------------

void QPlotBox::erasePlotBox()
{
    QPainter paint(pixelCanvas);
    brush.setColor(QColor(plot_config.plot_back_color));
    brush.setStyle(Qt::SolidPattern);
    paint.setBrush(brush);
    paint.drawRect(pixelCanvas->rect());
}



void QPlotBox::plotGrid(double x_from, double x_to, double y_from, double y_to, bool x_is_us){
    plot_config.plot_grid_x_starts = x_from;
    plot_config.plot_grid_x_ends = x_to;
    plot_config.plot_grid_y_starts = y_from;
    plot_config.plot_grid_y_ends = y_to;
    plot_config.plot_grid_x_is_us = x_is_us;
    plotGrid();
}

void QPlotBox::plotGrid(){
    int w = pixelCanvas->width();
    int h = pixelCanvas->height();
    pen.setColor(QColor(plot_config.plot_grid_line_color));
    pen.setWidth(0);
    pen.setStyle((Qt::PenStyle)plot_config.plot_grid_line_style);
    QPainter paint(pixelCanvas);
    paint.setFont(QFont(plot_config.plot_grid_font_name, plot_config.plot_grid_font_size));
    paint.setPen(pen);

    // Valor que separa dos grillas
    double grid_x_value = (double)(plot_config.plot_grid_x_ends - plot_config.plot_grid_x_starts)/plot_config.plot_grid_x_count;
    double grid_y_value = (double)(plot_config.plot_grid_y_ends - plot_config.plot_grid_y_starts)/plot_config.plot_grid_y_count;
    //Cantidad de grillas que se van a mostrar segun el zoom establecido
    uint32_t grid_x_total_count = (plot_config.plot_grid_x_count)*plot_config.plot_x_zoom;
    uint32_t grid_y_total_count = (plot_config.plot_grid_y_count)*plot_config.plot_y_zoom;

    for(uint32_t i = 0 ; i<grid_y_total_count+1;i++){
        uint32_t curr_h = (h-26)-(((h-26)*i)/grid_y_total_count)+4;
        QString val = QString::number((double)(plot_config.plot_grid_x_starts + (grid_y_value*i)), 'g', 2) + plot_config.plot_grid_y_unit;
        paint.drawLine(plot_config.plot_grid_font_size*0.8*(val.size()+1), curr_h, w, curr_h);
        paint.drawText(4, curr_h+(plot_config.plot_grid_font_size/2), val);
    }

    // Escalo los valores de tiempo a algo mas lejible, hr son microsegundos siempre
    if(plot_config.plot_grid_x_is_us){
        uint64_t hr = plot_config.plot_grid_x_ends - plot_config.plot_grid_x_starts;
        uint8_t hrfactor = 0;
        QString tunits[] = {"us", "ms", "s"};
        while (hr > 1000) {
            hr /= 1000;
            hrfactor++;
            grid_x_value /= 1000;
        }

        for(uint32_t i = 1 ; i<grid_x_total_count+1;i++){
            uint32_t curr_w = ((w*i)/grid_x_total_count)+4;
            paint.drawLine(curr_w, 0 , curr_w, h - (plot_config.plot_grid_font_size*2));
            paint.drawText(curr_w-10, h-2, QString::number((int)(grid_x_value*i)) + tunits[hrfactor]);
        }
    }else{
        for(uint32_t i = 1 ; i<grid_x_total_count+1;i++){
            uint32_t curr_w = ((w*i)/grid_x_total_count)+4;
            paint.drawLine(curr_w, 0 , curr_w, h - (plot_config.plot_grid_font_size*2));
            paint.drawText(curr_w-10, h-2, QString::number((int)(grid_x_value*i)));
        }
    }
}



//------------------------------------------------------------------------------
void QPlotBox::plotGraph(double values[], int count, double offset){}

//------------------------------------------------------------------------------

void QPlotBox::plotSetStyle(QString line_color, int line_style, QString font_name, int  font_size, QString back_color, QString y_uinit, bool x_is_us){
    plot_config.plot_grid_line_color = line_color;
    plot_config.plot_grid_line_style = line_style;
    plot_config.plot_grid_font_name = font_name;
    plot_config.plot_grid_font_size = font_size;
    plot_config.plot_back_color = back_color;
    plot_config.plot_grid_y_unit = y_uinit;
    plot_config.plot_grid_x_is_us = x_is_us;
}

void QPlotBox::plotSetZoom(double x_zoom, double y_zoom){
    plot_config.plot_x_zoom = x_zoom;
    plot_config.plot_y_zoom = y_zoom;
}

//------------------------------------------------------------------------------
QPixmap *QPlotBox::getCanvas(){
    return pixelCanvas;
}

//------------------------------------------------------------------------------
void QPlotBox::mousePressEvent(QMouseEvent *event){
    emit (plotBoxMouseDown(event->button(),event->pos().x(), event->pos().y()));
}

//------------------------------------------------------------------------------
void QPlotBox::mouseReleaseEvent(QMouseEvent *event){
    emit (plotBoxMouseUp(event->button(),event->pos().x(), event->pos().y()));
}

//------------------------------------------------------------------------------
void QPlotBox::mouseMoveEvent(QMouseEvent *event){
    emit (plotBoxMouseMove(event->button(),event->pos().x(), event->pos().y()));
}
