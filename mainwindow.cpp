#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QHeaderView>
#include <cmath>

Tomasulo global_tomasulo;

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *hbox = new QHBoxLayout;
    QVBoxLayout *vbox = new QVBoxLayout;

    vbox->addWidget(new QLabel("指令："));
    this->instList = new QTableWidget(this);
    QStringList header;
    header<<"指令"<<"已发射"<<"已结束";
    this->instList->setColumnCount(3);
    this->instList->setRowCount(0);
    this->instList->setHorizontalHeaderLabels(header);
    this->instList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->instList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vbox->addWidget(this->instList);
    hbox->addLayout(vbox);

    vbox = new QVBoxLayout;
    vbox->addWidget(new QLabel("当前周期："));
    this->cycleNumber = new QLCDNumber(this);
    this->cycleNumber->setMinimumHeight(60);
    vbox->addWidget(this->cycleNumber);
    vbox->addWidget(new QLabel("编辑指令："));
    this->instEdit = new QLineEdit(this);
    vbox->addWidget(this->instEdit);

    QPushButton *loadButton = new QPushButton("读入程序", this);
    QPushButton *addButton = new QPushButton("添加指令", this);
    QPushButton *nextButton = new QPushButton("单步执行", this);
    QPushButton *continueButton = new QPushButton("继续执行", this);
    QPushButton *multiButton = new QPushButton("执行多步", this);


    QHBoxLayout *hboxt = new QHBoxLayout;
    hboxt->addWidget(loadButton);
    hboxt->addWidget(addButton);
    vbox->addLayout(hboxt);

    hboxt = new QHBoxLayout;
    hboxt->addWidget(nextButton);
    hboxt->addWidget(continueButton);
    vbox->addLayout(hboxt);

    hboxt = new QHBoxLayout;
    hboxt->addWidget(multiButton);
    this->numSpin = new QSpinBox(this);
    this->numSpin->setMinimum(1);
    hboxt->addWidget(numSpin);
    vbox->addLayout(hboxt);

    vbox->addWidget(new QLabel("寄存器状态："));
    this->regList = new QTableWidget(this);
    header.clear();
    header<<"地址"<<"值";
    this->regList->setColumnCount(2);
    this->regList->setRowCount(16);
    this->regList->setHorizontalHeaderLabels(header);
    this->regList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->regList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vbox->addWidget(this->regList);

    hbox->addLayout(vbox);

    vbox = new QVBoxLayout;
    vbox->addWidget(new QLabel("内存状态："));
    this->memList = new QTableWidget(this);
    header.clear();
    header<<"地址"<<"值";
    this->memList->setColumnCount(2);
    this->memList->setRowCount(0);
    this->memList->setHorizontalHeaderLabels(header);
    this->memList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //this->memList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vbox->addWidget(this->memList);
    QPushButton *showallButton = new QPushButton("显示所有", this);
    QPushButton *shownzButton = new QPushButton("显示非零", this);
    showall = true;
    hboxt = new QHBoxLayout;
    hboxt->addWidget(showallButton);
    hboxt->addWidget(shownzButton);
    vbox->addLayout(hboxt);
    hbox->addLayout(vbox);

    layout->addLayout(hbox);
    layout->setStretchFactor(hbox, 60);

    layout->addWidget(new QLabel("保留站状态："));
    this->rsList = new QTableWidget(this);
    header.clear();
    header<<"剩余周期数"<<"名称"<<"繁忙"<<"操作"<<"Vj"<<"Vk"<<"Qj"<<"Qk";
    this->rsList->setColumnCount(8);
    this->rsList->setRowCount(11);
    this->rsList->setHorizontalHeaderLabels(header);
    this->rsList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->rsList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(this->rsList);
    this->setLayout(layout);

    this->tomasulo = &global_tomasulo;

    connect(addButton, SIGNAL(clicked(bool)), this, SLOT(addInst()));
    connect(loadButton, SIGNAL(clicked(bool)), this, SLOT(loadInst()));
    connect(nextButton, SIGNAL(clicked(bool)), this, SLOT(nextInst()));
    connect(continueButton, SIGNAL(clicked(bool)), this, SLOT(continueInst()));
    connect(multiButton, SIGNAL(clicked(bool)), this, SLOT(multiInst()));
    connect(showallButton, SIGNAL(clicked(bool)), this, SLOT(showAll()));
    connect(shownzButton, SIGNAL(clicked(bool)), this, SLOT(showNZ()));
    connect(this->memList, SIGNAL(cellChanged(int,int)), this, SLOT(changeMemory(int,int)));

    this->updateMemory();
    this->updateRegister();
    this->updateRStation();
}

void MainWindow::showAll()
{
    this->showall = true;
    this->updateMemory();
}

void MainWindow::showNZ()
{
    this->showall = false;
    this->updateMemory();
}

void MainWindow::updateMemory()
{
    const size_t MAX_MEMORY = 1 << 12;
    this->memList->setRowCount(MAX_MEMORY);
    size_t num=0;
    for(size_t i=0; i<MAX_MEMORY;i++)
    {
        double value = this->tomasulo->get_memory(i);
        if(fabs(value) < 1e-9 && !this->showall)
            continue;
        QString str = QString("%1").arg(i,4,10,QLatin1Char('0'));
        QTableWidgetItem *item = new QTableWidgetItem(str);
        item->setFlags(item->flags() & (~Qt::ItemIsEditable));
        this->memList->setItem(num, 0, item);
        this->memList->setItem(num, 1, new QTableWidgetItem(QString::number(value)));
        num++;
    }
    this->memList->setRowCount(num);
}

void MainWindow::updateRegister()
{
    const size_t MAX_REG = 16;
    for(size_t i=0; i<MAX_REG;i++)
    {
        double value = this->tomasulo->get_register(i);
        QString str = "R"+QString::number(i);
        this->regList->setItem(i, 0, new QTableWidgetItem(str));
        if(value > 1e99)
            this->regList->setItem(i, 1, new QTableWidgetItem(QString("Waiting...")));
        else
            this->regList->setItem(i, 1, new QTableWidgetItem(QString::number(value)));
    }
}

void MainWindow::updateRStation()
{
    const size_t MAX_RS = 11;
    for(size_t i=0; i<MAX_RS;i++)
    {
        const ReservationStation &s = this->tomasulo->station[i];
        QString a[] = {"×", "√"};
        const char *name[] = {"ADD1", "ADD2", "ADD3", "MUL/DIV1", "MUL/DIV2", "LOAD1", "LOAD2", "LOAD3", "STORE1", "STORE2", "STORE3"};
        this->rsList->setItem(i, 0, new QTableWidgetItem(QString::number(s.remain_time)));
        this->rsList->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(name[i])));
        this->rsList->setItem(i, 2, new QTableWidgetItem(a[s.busy]));
        this->rsList->setItem(i, 3, new QTableWidgetItem(QString::number(s.OP)));
        this->rsList->setItem(i, 4, new QTableWidgetItem(QString::number(s.Vj)));
        this->rsList->setItem(i, 5, new QTableWidgetItem(QString::number(s.Vk)));
        this->rsList->setItem(i, 6, new QTableWidgetItem(QString::number(s.Qj?s.Qj-this->tomasulo->station:-1)));
        this->rsList->setItem(i, 7, new QTableWidgetItem(QString::number(s.Qk?s.Qk-this->tomasulo->station:-1)));
    }
}

void MainWindow::changeMemory(int r, int c)
{
    size_t index = this->memList->item(r, 0)->text().toInt();
    bool ok = false;
    double value = this->memList->item(r, c)->text().toDouble(&ok);
    if(ok)
    {
        if(fabs(this->tomasulo->get_memory(index) - value) < 1e-9) return;
        this->tomasulo->set_memory(index, value);
        //this->updateMemory();
    }
}

void MainWindow::updateInstList()
{
    this->instList->setRowCount(tomasulo->instruction.size());
    for(size_t i=0; i<tomasulo->instruction.size();i++)
    {
        QString a[] = {"×", "√"};
        this->instList->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(tomasulo->instruction[i].ins_str)));
        this->instList->setItem(i, 1, new QTableWidgetItem(a[tomasulo->instruction[i].shoot_time <= tomasulo->clock && tomasulo->instruction[i].shoot_time >= 0]));
        this->instList->setItem(i, 2, new QTableWidgetItem(a[tomasulo->instruction[i].finish_time <= tomasulo->clock && tomasulo->instruction[i].finish_time >= 0]));
    }
    //this->instList->resizeColumnsToContents();
}

void MainWindow::updateCycleNumber()
{
    this->cycleNumber->display(this->tomasulo->clock);
}

void MainWindow::nextInst()
{
    if(!this->tomasulo->is_finish()) this->tomasulo->step();
    this->updateCycleNumber();
    this->updateInstList();
    this->updateMemory();
    this->updateRegister();
    this->updateRStation();
}

void MainWindow::continueInst()
{
    while(!this->tomasulo->is_finish())
    {
        this->nextInst();
    }
}

void MainWindow::multiInst()
{
    for(int i=0;!this->tomasulo->is_finish() && i<this->numSpin->value();i++)
    {
        this->nextInst();
    }
}

void MainWindow::addInst(const QString *str)
{
    QString edit = this->instEdit->text();
    if(str == NULL) str = &edit;
    try
    {
        this->tomasulo->addInstruction(Instruction(str->toStdString()));
    }
    catch(int x)
    {
        QMessageBox::warning(this, "指令错误", "\""+*str+"\"是非法指令");
    }
    if(str == &edit) updateInstList();
}

void MainWindow::loadInst()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",tr("All File(*.*)"));
    if (fileName.isEmpty()) return;
    QFile fi(fileName);
    if (!fi.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    while(!fi.atEnd())
    {
        QString line(fi.readLine());
        line = line.simplified();
        this->addInst(&line);
    }
    fi.close();
    updateInstList();
}

MainWindow::~MainWindow()
{

}
