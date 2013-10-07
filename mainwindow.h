#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMultiMap>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

namespace Ui {
class MainWindow;
}

typedef QMultiMap<QString, QString> MatchesMap;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void ReadFiles();
    void Save();
    
private:
    Ui::MainWindow *ui;

    MatchesMap m_matches;
};

#endif // MAINWINDOW_H
