#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

// List of Canadian provinces for validation
const QStringList CANADIAN_PROVINCES = {"AB", "BC", "MB", "NB", "NL", "NS", "NT", "NU", "ON", "PE", "QC", "SK", "YT"};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleWeatherData);

    // Set validators for city and province to accept upper and lower case letters, white space, ".", "-", and "'"
    QRegularExpression regExp("[a-zA-Z\\s\\.\\-']+");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regExp, this);
    ui->txtCity->setValidator(validator);
    ui->txtProvince->setValidator(validator);

    // Set validator for postal code to accept proper Canadian postal code format
    QRegularExpression postalCodeRegExp("^[A-Za-z]\\d[A-Za-z] ?\\d[A-Za-z]\\d$");
    QRegularExpressionValidator *postalCodeValidator = new QRegularExpressionValidator(postalCodeRegExp, this);
    ui->txtPostalCode->setValidator(postalCodeValidator);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_btnWeather_clicked()
{


    QString city = ui->txtCity->text().trimmed();
    QString province = ui->txtProvince->text().trimmed();
    QString postalCode = ui->txtPostalCode->text().trimmed();

    // Validate that both city and province are filled, and contain only valid characters
    if ((city.isEmpty() || province.isEmpty()) && postalCode.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "You must either provide both City and Province or a Postal Code.");
        return;
    }

    if ((!city.isEmpty() || !province.isEmpty()) && !QRegularExpression("^[a-zA-Z\\s\\.\\-']+$").match(city + province).hasMatch()) {
        QMessageBox::warning(this, "Input Error", "City and Province fields must only contain letters, spaces, dots, hyphens, and apostrophes.");
        return;
    }

    // Validate the postal code format
    if (!postalCode.isEmpty() && !QRegularExpression("^[A-Za-z]\\d[A-Za-z] ?\\d[A-Za-z]\\d$").match(postalCode).hasMatch()) {
        QMessageBox::warning(this, "Input Error", "Postal code must be in the format A1A 1A1.");
        return;
    }

    // Validate that the province is a Canadian province
    if (!province.isEmpty() && !CANADIAN_PROVINCES.contains(province)) {
        QMessageBox::warning(this, "Input Error", "The province must be a valid Canadian province abbreviation.");
        return;
    }

    // Validate the city name
    if (!city.isEmpty() && !city.contains(city)) { QMessageBox::warning(this, "Input Error", "The city must be a valid Canadian city.");
        return; }

    int days = ui->radioBtn1Day->isChecked() ? 1 : 7;
    QString urlString;
    if (!postalCode.isEmpty()) {
        urlString = QString("https://api.weatherbit.io/v2.0/forecast/daily?key=47646613f1c442ab94df5742049f2d3b&units=metric&days=%1&postal_code=%2").arg(days).arg(postalCode);
    } else {
        urlString = QString("https://api.weatherbit.io/v2.0/forecast/daily?key=47646613f1c442ab94df5742049f2d3b&units=metric&days=%1&city=%2,%3").arg(days).arg(city).arg(province);
    }

    QUrl url(urlString);
    QNetworkRequest request(url);
    networkManager->get(request);
}

void MainWindow::handleWeatherData(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray dataArray = jsonObj["data"].toArray();

        QString weatherInfo = "<style>"
                              "table {width: 100%; height: 100%; border-collapse: collapse; table-layout: fixed; font-size: 20px}"
                              "th, td {border: 1px solid black; padding: 8px; text-align: center;}"
                              "th {background-color: #f2f2f2;}"
                              "</style>";

        weatherInfo += "<table>";

        // Create headers for the first column and for each day
        weatherInfo += "<tr><th></th>";
        for (const QJsonValue &value : dataArray) {
            QJsonObject dataObj = value.toObject();
            QString date = dataObj["datetime"].toString();
            weatherInfo += QString("<th>%1</th>").arg(date);
        }
        weatherInfo += "</tr>";

        // Add rows for Current, Min Temp, Max Temp, POP, and Description
        QStringList rowHeaders = {"Temp (°C)", "Min Temp (°C)", "Max Temp (°C)", "POP (%)", "Description"};
        for (const QString &header : rowHeaders) {
            weatherInfo += QString("<tr><th>%1</th>").arg(header);
            for (const QJsonValue &value : dataArray) {
                QJsonObject dataObj = value.toObject();
                if (header == "Temp (°C)") {
                    double temp = dataObj["temp"].toDouble();
                    weatherInfo += QString("<td>%1</td>").arg(temp);
                } else if (header == "Min Temp (°C)") {
                    double minTemp = dataObj["min_temp"].toDouble();
                    weatherInfo += QString("<td>%1</td>").arg(minTemp);
                } else if (header == "Max Temp (°C)") {
                    double maxTemp = dataObj["max_temp"].toDouble();
                    weatherInfo += QString("<td>%1</td>").arg(maxTemp);
                } else if (header == "POP (%)") {
                    double pop = dataObj["pop"].toDouble();
                    weatherInfo += QString("<td>%1</td>").arg(pop);
                } else if (header == "Description") {
                    QJsonObject weatherObj = dataObj["weather"].toObject();
                    QString description = weatherObj["description"].toString();
                    weatherInfo += QString("<td>%1</td>").arg(description);
                }
            }
            weatherInfo += "</tr>";
        }

        weatherInfo += "</table>";

        ui->outputWeather->setHtml(weatherInfo); // Display the response in a QTextEdit with HTML formatting
    } else {
        ui->outputWeather->setText("Error: " + reply->errorString());
    }
    reply->deleteLater();
}


void MainWindow::on_btnReset_clicked() {
    ui->txtCity->clear();
    ui->txtProvince->clear();
    ui->txtPostalCode->clear();
    ui->outputWeather->clear();
    ui->radioBtn1Day->setChecked(true);
}

