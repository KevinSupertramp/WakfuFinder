#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->Parse, SIGNAL(clicked()), this, SLOT(ParseSource()));
    connect(ui->Start, SIGNAL(clicked()), this, SLOT(ReadFiles()));
    connect(ui->Save, SIGNAL(clicked()), this, SLOT(Save()));
    connect(ui->Empty, SIGNAL(clicked()), this, SLOT(EmptyList()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::EmptyList()
{
    int count = ui->treeWidget->topLevelItemCount();
    for (int i = 0; i < count; ++i)
        qDeleteAll(ui->treeWidget->topLevelItem(i)->takeChildren());
}

void MainWindow::ReadFiles()
{
    QDir dir;
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    QString nameFilter = "*.java";
    dir.setNameFilters(nameFilter.split(" "));

    QFileInfoList list = dir.entryInfoList();
    int filesNumber = list.size();

    if (filesNumber == 0)
        return;

    ui->progressBar->setMaximum(filesNumber);
    ui->lineEdit->setText(QString::number(filesNumber));

    QEventLoop loop;
    QTimer timer;

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    timer.start(1000);
    EmptyList();
    loop.exec();

    for (int i = 0; i < filesNumber; ++i)
    {
        QFileInfo info = list.at(i);
        QFile file(info.fileName());

        if (!file.open(QIODevice::ReadOnly))
            qDebug() << "Error while opening " << info.fileName() << " : " << file.errorString();

        QTextStream in(file.readAll());
        QString line;
        QString pattern = ui->Search->text();

        do
        {
            line = in.readLine();

            if (pattern.isEmpty())
            {
                if (line.contains("public int getId()"))
                {
                    QString opcode;

                    if (line.contains("return"))
                        opcode = line;
                    else
                    {
                        opcode = in.readLine();
                        if (!opcode.contains("return"))
                            opcode = in.readLine();

                        if (!opcode.contains("return"))
                        {
                            qDebug() << "Can't find return string in file " << info.fileName();
                            continue;
                        }
                    }

                    opcode = opcode.split("return ").at(1);
                    opcode = opcode.split(";").at(0);

                    if (!opcode.toInt())
                    {
                        qDebug() << "Opcode " << opcode << " isn't a number INT. " << info.fileName();
                        continue;
                    }

                    m_opcodes.insertMulti(opcode.toInt(), info.fileName());
                }
            }
            else
            {
                if (line.contains(pattern))
                    m_matches.insertMulti(line, info.fileName());
            }

        } while (!line.isNull());
    }

    quint32 i = 0;
    for (OpcodeMatchesMap::ConstIterator itr = m_opcodes.begin(); itr != m_opcodes.end(); ++itr)
    {
        if (itr.key() == 0)
            continue;

        QTreeWidgetItem* item = new QTreeWidgetItem;
        item->setText(0, QString::number(++i));
        item->setText(1, QString::number(itr.key()));
        item->setText(2, itr.value());

        ui->treeWidget->addTopLevelItem(item);
    }
}

void MainWindow::Save()
{
    QString filename = QFileDialog::getSaveFileName(this, "Sauvegarder sous...", "", "Fichier texte (*.txt)");

    if (filename.isNull())
        return;

    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Erreur !", QString("Le fichier %1 ne peut être écrit !").arg(filename));
        return;
    }

    QTextStream out(&file);
    QTreeWidgetItemIterator itr(ui->treeWidget);

    while (*itr)
    {
        out << (*itr)->text(0) << " | " << (*itr)->text(1) << " | " << (*itr)->text(2) << "\n";
        ++itr;
    }

    file.close();
}

// Parser
void MainWindow::ParseSource()
{
    QFileInfoList list = ReadFileList();
    int filesNumber = list.size();

    if (filesNumber == 0)
        return;

    ui->progressBar->setMaximum(filesNumber * 2);
    ui->lineEdit->setText(QString::number(filesNumber * 2));

    QEventLoop loop;
    QTimer timer;

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    timer.start(1000);
    EmptyList();
    loop.exec();

    ProcessSource(list, PROCESS_STEP_READ_CLASS);
    ProcessSource(list, PROCESS_STEP_REPLACE_CLASS);
}

QFileInfoList MainWindow::ReadFileList()
{
    QDir dir;
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    QString nameFilter = "*.java";
    dir.setNameFilters(nameFilter.split(" "));

    return dir.entryInfoList();
}

void MainWindow::ProcessSource(QFileInfoList list, ProcessStep step)
{
    int filesNumber = list.size();

    for (int i = 0; i < filesNumber; ++i)
    {
        QFileInfo info = list.at(i);
        QFile file(info.fileName());

        if (!file.open(QIODevice::ReadWrite))
            qDebug() << "Error while opening " << info.fileName() << " : " << file.errorString();

        QTextStream in(file.readAll());
        QString line;

        // Faut prendre en compte les interfaces : public abstract interface dKW extends dFD
        do
        {
            line = in.readLine();

            // First get all class
            if (step == PROCESS_STEP_READ_CLASS)
            {
                QString _class;

                if (line.contains("class "))
                    _class = line.split("class ").at(1);
                else if (line.contains("interface "))
                    _class = line.split("interface ").at(1);
                else
                    continue;

                if (_class.contains("extends"))
                    _class = _class.split(" extends").at(0);

                if (_class.length() > 3)
                    continue;

                QString newName = info.fileName().split(".").at(0);
                m_classes.insert(_class, newName);
            }
            else if (step == PROCESS_STEP_REPLACE_CLASS)
            {
                if (line.contains("class "))
                {
                    QString _class = line.split("class ").at(1);

                    if (_class.contains("extends"))
                        _class = _class.split(" extends").at(0);

                    if (_class.length() > 3)
                        continue;

                    QString newClass = m_classes.value(_class, QString());

                    if (newClass.isEmpty())
                    {
                        qDebug() << "[newClass] Class " << _class << " not found in map !!";
                        continue;
                    }

                    line.replace(_class, newClass);
                }

                if (line.contains("extends"))
                {
                    QStringList list;
                    QString extendedClass = line.split(" extends ").at(1);

                    if (extendedClass.contains(","))
                        list = extendedClass.split(", ");

                    if (!list.empty())
                    {
                        int size = list.size();
                        for (int i = 0; i < size; ++i)
                        {
                            extendedClass = list.at(i);

                            if (extendedClass.length() > 3)
                                continue;

                            QString newExtendedClass = m_classes.value(extendedClass, QString());
                            if (newExtendedClass.isEmpty())
                            {
                                qDebug() << "[extendedClass] Class " << extendedClass << " not found in map !!";
                                continue;
                            }

                            line.replace(extendedClass, newExtendedClass);
                        }
                    }
                    else
                    {
                        if (extendedClass.length() > 3)
                            continue;

                        QString newExtendedClass = m_classes.value(extendedClass, QString());
                        if (newExtendedClass.isEmpty())
                        {
                            qDebug() << "[extendedClass] Class " << extendedClass << " not found in map !! File : " << info.fileName();
                            continue;
                        }

                        line.replace(extendedClass, newExtendedClass);
                    }
                }
            }
        } while (!line.isNull());

        ui->progressBar->setValue(ui->progressBar->value() + 1);
        ui->lineEdit->setText(QString::number((ui->lineEdit->text().toInt() - 1)));
    }
}
