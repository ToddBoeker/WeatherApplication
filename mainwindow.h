#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void on_btnWeather_clicked();
    void on_btnReset_clicked();
    //void on_fetchButton_clicked();
    void handleWeatherData(QNetworkReply* reply);



private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;


};


#endif // MAINWINDOW_H
