#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QString>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QPalette>
#include <QPixmap>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateBackground(const QString& weatherCondition);
    void fetchWeatherData();
    void fetchCurrentLocation();
    void handleWeatherData(QNetworkReply* reply);
    void handleLocationResponse(QNetworkReply* reply);

private slots:
    void fetchWeatherDataSlot();
    void on_btnWeather_clicked();
    void on_btnReset_clicked();
    void displayWeatherData(const QJsonObject &jsonObj);
    void testWeatherData();
    void clearForecastDisplay();
    void handleLocationData(QNetworkReply *reply);
    void normalizeInput(QString text);


private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QNetworkAccessManager *locationNetworkManager;
    QStringList bgImages;
    QString determineBackgroundImage(const QString &weather);
    void setAppBackground(const QString &imagePath);

    QLineEdit *txtCity;
    QLineEdit *txtProvince;
    QLineEdit *txtPostalCode;
    QPushButton *btnWeather;
};

#endif // MAINWINDOW_H
