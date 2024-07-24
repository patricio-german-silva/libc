#ifndef QPLOTBOX_H
#define QPLOTBOX_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

class QPlotBox : public QWidget
{
    Q_OBJECT
public:
    explicit QPlotBox(int aWidth = 200, int aHeigth = 200, QWidget *parent = nullptr);

    void setWidth(int awidth);
    void setHeight(int aheight);

    // Paint functions
    void erasePlotBox();
    void plotGrid();
    void plotGrid(double x_from, double x_to, double y_from, double y_to, bool x_is_us = false);
    void plotGraph(double values[], int count, double offset = 0.0);
    void plotSetStyle(QString line_color = "gray", int line_style = 3, QString font_name = "Arial", int  font_size = 8,
                      QString back_color = "Black", QString y_uinit = "", bool x_is_us = true);
    void plotSetZoom(double x_zoom, double y_zoom);
    QPixmap *getCanvas(void);

signals:
    void plotBoxMouseMove(Qt::MouseButton button, int x, int y);
    void plotBoxMouseDown(Qt::MouseButton button, int x, int y);
    void plotBoxMouseUp(Qt::MouseButton button, int x, int y);


public slots:

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    typedef struct{
        int32_t plot_grid_y_count;
        double plot_grid_y_starts;
        double plot_grid_y_ends;
        QString plot_grid_y_unit;
        int32_t plot_grid_x_count;
        double plot_grid_x_starts;
        double plot_grid_x_ends;
        bool plot_grid_x_is_us;
        QString plot_grid_line_color;
        int32_t plot_grid_line_style;
        QString plot_grid_font_name;
        int32_t plot_grid_font_size;
        QString plot_back_color;
        double plot_x_zoom;    // Proporcion de zoom
        double plot_y_zoom;
        double x_cursors[2];
        double y_cursors[2];
        bool show_x_cursors;
        bool show_y_cursors;
    }_config;

    QPixmap *pixelCanvas;
    QPen pen;
    QBrush brush;
    _config plot_config;
};

#endif // QPLOTBOX_H
