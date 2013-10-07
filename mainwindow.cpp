#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->Start, SIGNAL(clicked()), this, SLOT(ReadFiles()));
    connect(ui->Save, SIGNAL(clicked()), this, SLOT(Save()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ReadFiles()
{
    QDir dir;
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    QString nameFilter = "*.java *.class";
    dir.setNameFilters(nameFilter.split(" "));

    QFileInfoList list = dir.entryInfoList();
    int filesNumber = list.size();

    if (filesNumber == 0)
        return;

    ui->progressBar->setMaximum(filesNumber);
    ui->lineEdit->setText(QString::number(filesNumber));
    ui->treeWidget->clear();

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
                    m_matches.insertMulti(opcode.split(";").at(0), info.fileName());
                }
            }
            else
            {
                if (line.contains(pattern))
                    m_matches.insertMulti(line, info.fileName());
            }

        } while (!line.isNull());

        ui->progressBar->setValue(i + 1);
        ui->lineEdit->setText(QString::number((filesNumber -1) - i));
    }

    quint32 i = 0;
    for (MatchesMap::ConstIterator itr = m_matches.begin(); itr != m_matches.end(); ++itr)
    {
        if (itr.key() == 0)
            continue;

        QTreeWidgetItem* item = new QTreeWidgetItem;
        item->setText(0, QString::number(++i));
        item->setText(1, itr.key());
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
