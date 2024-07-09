#ifndef SCOPE_H
#define SCOPE_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Scope; }
QT_END_NAMESPACE

class Scope : public QMainWindow
{
    Q_OBJECT

public:
    Scope(QWidget *parent = nullptr);
    ~Scope();

private:
    Ui::Scope *ui;
};
#endif // SCOPE_H
