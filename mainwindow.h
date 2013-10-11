#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMultiMap>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

namespace Ui {
class MainWindow;
}

enum ProcessStep
{
    PROCESS_STEP_READ_CLASS,
    PROCESS_STEP_REPLACE_CLASS
};

typedef QMultiMap<QString, QString> StringMatchesMap;
typedef QMultiMap<int, QString> OpcodeMatchesMap;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QFileInfoList ReadFileList();
    void ProcessSource(QFileInfoList list, ProcessStep step);

public slots:
    void EmptyList();
    void ReadFiles();
    void Save();

    void ParseSource();
    
private:
    Ui::MainWindow *ui;

    StringMatchesMap m_matches;
    StringMatchesMap m_classes;
    OpcodeMatchesMap m_opcodes;
};

#endif // MAINWINDOW_H
