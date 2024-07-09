#include "scope.h"
#include "ui_scope.h"

Scope::Scope(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Scope)
{
    ui->setupUi(this);
}

Scope::~Scope()
{
    delete ui;
}

