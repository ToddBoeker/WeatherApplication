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
#include <QFile>
#include <QDir>
#include <QPixmap>
#include <QPalette>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QStringList>
#include <QMap>
#include <QTime>


using namespace std;


// List of Canadian provinces for validation
const QStringList CANADIAN_PROVINCES = {"AB", "BC", "MB", "NB", "NL", "NS", "NT", "NU", "ON", "PE", "QC", "SK", "YT"};

const QMap<QString, QString> PROVINCE_NAME_TO_ABBREVIATION = {
    {"Alberta", "AB"},
    {"British Columbia", "BC"},
    {"Manitoba", "MB"},
    {"New Brunswick", "NB"},
    {"Newfoundland and Labrador", "NL"},
    {"Nova Scotia", "NS"},
    {"Northwest Territories", "NT"},
    {"Nunavut", "NU"},
    {"Ontario", "ON"},
    {"Prince Edward Island", "PE"},
    {"Quebec", "QC"},
    {"Saskatchewan", "SK"},
    {"Yukon", "YT"}
};


#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    networkManager(new QNetworkAccessManager(this)),
    locationNetworkManager(new QNetworkAccessManager(this)) {
    ui->setupUi(this);
    fetchWeatherDataSlot();

    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleWeatherData);
    connect(ui->btnCurrentLocation, &QPushButton::clicked, this, &MainWindow::fetchWeatherDataSlot);

    // Set up the validator
    QRegularExpression regExp("[a-zA-Z\\s\\.\\-']+");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regExp, this);
    ui->txtCity->setValidator(validator);
    ui->txtProvince->setValidator(validator);

    QRegularExpression postalCodeRegExp("^[A-Za-z]\\d[A-Za-z] ?\\d[A-Za-z]\\d$");
    QRegularExpressionValidator *postalCodeValidator = new QRegularExpressionValidator(postalCodeRegExp, this);
    ui->txtPostalCode->setValidator(postalCodeValidator);


    QString greetingMessage;
    QTime currentTime = QTime::currentTime();
    if (currentTime.hour() < 12) {
        greetingMessage = "Good Morning!";
    } else if (currentTime.hour() < 16) {
        greetingMessage = "Good Afternoon!";
    } else {
        greetingMessage = "Good Evening!";
    }

    QString finalMessage = QString("%1 Get the latest forecast for your area").arg(greetingMessage);
    ui->lblGreeting->setText(finalMessage);
    ui->lblGreeting->setStyleSheet("QLabel { color: gold; }");


    QPixmap background(":/images/Default.jpeg");
    background = background.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, background);
    this->setPalette(palette);


    // Ensure QLabel is large enough
    ui->lblGreeting->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->lblGreeting->setMinimumSize(200, 50);
    ui->lblGreeting->setWordWrap(true);
    ui->lblGreeting->setAlignment(Qt::AlignCenter);

    QFont font = ui->lblGreeting->font();
    font.setPointSize(12);
    ui->lblGreeting->setFont(font);
    ui->lblGreeting->setStyleSheet("QLabel { color : gold; }");

    // Connect the dedicated manager for IPInfo location data
    connect(locationNetworkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleLocationResponse);

    // Connect input normalization for city and province
    connect(ui->txtCity, &QLineEdit::textChanged, this, &MainWindow::normalizeInput);
    connect(ui->txtProvince, &QLineEdit::textChanged, this, &MainWindow::normalizeInput);

    // Initialize your background images
    bgImages = {
        ":/images/Clear.jpeg",      // 0 Clear
        ":/images/Clouds.jpeg",     // 1 Clouds
        ":/images/Rain.jpg",        // 2 Rain
        ":/images/Snow.jpeg",       // 3 Snow
        ":/images/Fog.jpeg",        // 4 Fog
        ":/images/Storm.jpeg",      // 5 Storm
        ":/images/Wind.jpg",        // 6 Wind
        ":/images/Freezing_Rain.jpeg", // 7 Freezing Rain
        ":/images/Night.jpeg"       // 8 Night
    };

}

void MainWindow::fetchWeatherDataSlot() {
    QString city = ui->txtCity->text();
    QString province = ui->txtProvince->text();
    QString postalCode = ui->txtPostalCode->text();

    QString url;
    if (!postalCode.isEmpty()) {
        url = QString("https://api.weatherbit.io/v2.0/current?postal_code=%1&key=f44722b654a9490e8ddd26c1a152aa60").arg(postalCode);
    } else if (!city.isEmpty() && !province.isEmpty()) {
        url = QString("https://api.weatherbit.io/v2.0/current?city=%1&state=%2&key=f44722b654a9490e8ddd26c1a152aa60")
        .arg(city).arg(province);
    } else {
        return;
    }

    networkManager->get(QNetworkRequest(QUrl(url)));
}

void MainWindow::handleWeatherData(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        int days = ui->radioBtn1Day->isChecked() ? 1 : 7;
        QString day = QString::number(days);
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray dataArray = jsonObj["data"].toArray();

        // Save JSON data to file
        QDir dir;
        if (!dir.exists("Cities")) {
            dir.mkdir("Cities");
        }
        QString cityName = jsonObj["city_name"].toString();
        if (cityName.isEmpty()) {
            QString postalCode = ui->txtPostalCode->text().trimmed();
            cityName = postalCode;
        }
        cityName += day;
        QFile jsonFile("Cities/" + cityName + day + ".json");
        jsonFile.open(QFile::WriteOnly);
        jsonFile.write(jsonDoc.toJson());
        jsonFile.close();

        // Display the weather data
        displayWeatherData(jsonObj);

        // Determine the suitable background image
        QString weather = dataArray[0].toObject()["weather"].toObject()["description"].toString();
        qDebug() << "Weather Description from API:" << weather;
        qDebug() << "bgImages size:" << bgImages.size();  // Ensure bgImages size is logged

        QString bgImage = determineBackgroundImage(weather);
        qDebug() << "Selected Background Image:" << bgImage;  // Log selected background image

        // Set the background image
        setAppBackground(bgImage);
    }
    reply->deleteLater();
}


QString MainWindow::determineBackgroundImage(const QString &weather) {
    qDebug() << "Weather Description:" << weather;

    if (bgImages.size() < 9) {
        qDebug() << "Error: bgImages does not have enough elements!";
        return ":/images/Default.jpeg";  // Return an error image
    }

    QTime currentTime = QTime::currentTime();
    QTime duskTime(20, 0); // Assuming dusk is 8:00 PM
    bool isNight = currentTime > duskTime || currentTime < QTime(8, 0); // Night time is from 8:00 PM to 6:00 AM

    if (isNight) {
        if (weather.contains("Night", Qt::CaseInsensitive)) return bgImages[8]; // Assume bgImages[8] is your night clear image
    } else {
        if (weather.contains("Clear", Qt::CaseInsensitive)) return bgImages[0];
        if (weather.contains("Cloud", Qt::CaseInsensitive) || weather.contains("Overcast", Qt::CaseInsensitive)) return bgImages[1];
        if (weather.contains("Rain", Qt::CaseInsensitive) || weather.contains("Drizzle", Qt::CaseInsensitive) || weather.contains("Light rain", Qt::CaseInsensitive)) return bgImages[2];
        if (weather.contains("Snow", Qt::CaseInsensitive) || weather.contains("Sleet", Qt::CaseInsensitive)) return bgImages[3];
        if (weather.contains("Fog", Qt::CaseInsensitive) || weather.contains("Mist", Qt::CaseInsensitive) || weather.contains("Haze", Qt::CaseInsensitive)) return bgImages[4];
        if (weather.contains("Storm", Qt::CaseInsensitive) || weather.contains("Thunderstorm", Qt::CaseInsensitive)) return bgImages[5];
        if (weather.contains("Wind", Qt::CaseInsensitive) || weather.contains("Breeze", Qt::CaseInsensitive)) return bgImages[6];
        if (weather.contains("Freezing Rain", Qt::CaseInsensitive) || weather.contains("Ice Pellets", Qt::CaseInsensitive)) return bgImages[7];
    }

    // Default if no match
    return ":/images/Default.jpeg";
}




void MainWindow::setAppBackground(const QString &imagePath) {
    QPixmap bg(imagePath);
    if (bg.isNull()) {
        qDebug() << "Failed to load background image:" << imagePath;  // Log if the image fails to load
        return;
    }
    QPalette p = palette();
    p.setBrush(QPalette::Window, bg.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    setPalette(p);
}


void MainWindow::normalizeInput(QString text) {
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender());
    if (lineEdit) {
        lineEdit->setText(text.toUpper());
    }
}



MainWindow::~MainWindow() {
    QDir dir("Cities");
    // Check if the directory exists
    if (dir.exists())
    {
        dir.setFilter(QDir::Files);
        QFileInfoList fileList = dir.entryInfoList();

        // Iterate through each file and remove it
        for (const QFileInfo &fileInfo : fileList)
        {
            QFile::remove(fileInfo.absoluteFilePath());
        }

        // Remove the directory itself
        dir.rmdir("Cities");
    }
    delete ui;
}

void MainWindow::on_btnWeather_clicked() {
    clearForecastDisplay();


    QString city = ui->txtCity->text().trimmed().toUpper();
    QString province = ui->txtProvince->text().trimmed().toUpper();
    QString postalCode = ui->txtPostalCode->text().trimmed().toUpper();
    QString locationText;

    // Ensure that either 1-day or 7-day forecast option is selected
    if (!ui->radioBtn1Day->isChecked() && !ui->radioBtn7Day->isChecked()) {
        QMessageBox::warning(this, "Input Error", "Please select either a 1-day or 7-day forecast.");
        return;
    }

    if ((city.isEmpty() || province.isEmpty()) && postalCode.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "You must either provide both City and Province or a Postal Code.");
        return;
    }
    if ((!city.isEmpty() || !province.isEmpty()) && !QRegularExpression("^[a-zA-Z\\s\\.\\-']+$").match(city + province).hasMatch()) {
        QMessageBox::warning(this, "Input Error", "City and Province fields must only contain letters, spaces, dots, hyphens, and apostrophes.");
        return;
    }
    if (!postalCode.isEmpty() && !QRegularExpression("^[A-Za-z]\\d[A-Za-z] ?\\d[A-Za-z]\\d$").match(postalCode).hasMatch()) {
        QMessageBox::warning(this, "Input Error", "Postal code must be in the format A1A 1A1.");
        return;
    }
    if (!province.isEmpty() && !CANADIAN_PROVINCES.contains(province)) {
        QMessageBox::warning(this, "Input Error", "The province must be a valid Canadian province abbreviation.");
        return;
    }

    // Set the location text based on user input and update lblLocationInfo
    if (!postalCode.isEmpty()) {
        locationText = QString("Selected Location: Postal Code %1").arg(postalCode);
    } else if (!city.isEmpty() && !province.isEmpty()) {
        locationText = QString("Selected Location: %1, %2").arg(city, province);
    }




    ui->lblLocationInfo->setText(locationText); // Update lblLocationInfo only here

    // Get the forecast duration based on the selected radio button
    int days = ui->radioBtn1Day->isChecked() ? 1 : 7;

    // Construct the API URL based on the user input
    QString urlString;
    if (!postalCode.isEmpty()) {
        urlString = QString("https://api.weatherbit.io/v2.0/forecast/daily?key=f44722b654a9490e8ddd26c1a152aa60&units=metric&days=%1&postal_code=%2").arg(days).arg(postalCode);
    } else {
        urlString = QString("https://api.weatherbit.io/v2.0/forecast/daily?key=f44722b654a9490e8ddd26c1a152aa60&units=metric&days=%1&city=%2,%3").arg(days).arg(city).arg(province);
    }
    QString fileName;
    QString day = QString::number(days);

    if(city.isEmpty())
    {
        fileName = "Cities/" + postalCode;
    }
    else
    {
        fileName = "Cities/" + city;
    }
    fileName += "-" + day + "day";
    fileName += ".json";
    QFile file(fileName);

    if (file.exists())
    {
        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray fileData = file.readAll();
            file.close();
            // Parse the JSON content from the file
            QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData);
            QJsonObject jsonObj = jsonDoc.object();
            // Display the weather data from the file
            displayWeatherData(jsonObj);
        }
        else
        {
            QMessageBox::warning(this, "File Error", "Failed to open the file.");
        }
    }
    else
    {
        QUrl url(urlString);
        QNetworkRequest request(url);
        networkManager->get(request);
    }
}



void MainWindow::handleLocationResponse(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();

        QString city = jsonObj.value("city").toString();
        QString region = jsonObj.value("region").toString();

        qDebug() << "City:" << city;
        qDebug() << "Region (Full Name):" << region;

        QString regionAbbreviation = PROVINCE_NAME_TO_ABBREVIATION.value(region, region); // Defaults to full name if not found
        qDebug() << "Province Abbreviation:" << regionAbbreviation;

        if (!city.isEmpty() && !regionAbbreviation.isEmpty()) {
            ui->txtCity->setText(city);
            ui->txtProvince->setText(regionAbbreviation);
        } else {
            qDebug() << "Error: Missing city or region information in JSON.";
        }
    } else {
        qWarning() << "Network error: " << reply->errorString();
    }

    reply->deleteLater();
}



void MainWindow::fetchWeatherData() {
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &MainWindow::handleWeatherData);

    QString apiKey = "840faa516f9c455c962138131caa886c";
    QString city = "your_city";
    QString url = QString("http://api.openweathermap.org/data/2.5/weather?q=%1&appid=%2").arg(city).arg(apiKey);

    manager->get(QNetworkRequest(QUrl(url)));
}


// Assume displayWeatherData is part of the MainWindow class
void MainWindow::displayWeatherData(const QJsonObject &jsonObj)
{
    QJsonArray dataArray = jsonObj["data"].toArray();

    QLabel* dateLabels[7] = {ui->Date1, ui->Date2, ui->Date3, ui->Date4, ui->Date5, ui->Date6, ui->Date7};
    QLabel* iconLabels[7] = {ui->Icon1, ui->Icon2, ui->Icon3, ui->Icon4, ui->Icon5, ui->Icon6, ui->Icon7};
    QLabel* tempLabels[7] = {ui->Temp1, ui->Temp2, ui->Temp3, ui->Temp4, ui->Temp5, ui->Temp6, ui->Temp7};
    QLabel* maxtempLabels[7] = {ui->Maxtemp1, ui->Maxtemp2, ui->Maxtemp3, ui->Maxtemp4, ui->Maxtemp5, ui->Maxtemp6, ui->Maxtemp7};
    QLabel* mintempLabels[7] = {ui->MinTemp1, ui->MinTemp2, ui->MinTemp3, ui->MinTemp4, ui->Mintemp522, ui->MinTemp6, ui->MinTemp7};
    QFrame* days[7] = {ui->day1, ui->day2, ui->day3, ui->day4, ui->day5, ui->day6, ui->day7};

    int count = 0;

    for (const QJsonValue &value : dataArray)
    {
        QJsonObject dataObj = value.toObject();
        QString dateString = dataObj["datetime"].toString();

        // Convert the date string to QDate
        QDate date = QDate::fromString(dateString, "yyyy-MM-dd");

        // Format the date to a human-readable format
        QString formattedDate = date.toString("dddd, MMM d");

        double tem = dataObj["temp"].toDouble();
        double matem = dataObj["max_temp"].toDouble();
        double mitem = dataObj["min_temp"].toDouble();

        QString temp = QString::number(tem) + "°C";
        QString maxtemp = "Max:" + QString::number(matem) + "°C";
        QString mintemp = "Min:" + QString::number(mitem) + "°C";

        QJsonObject weatherObj = dataObj["weather"].toObject();
        QString image = ":/images/" + weatherObj["icon"].toString() + ".png";
        QPixmap pic(image);

        dateLabels[count]->setText(formattedDate);
        iconLabels[count]->setPixmap(pic);
        tempLabels[count]->setText(temp);
        maxtempLabels[count]->setText(maxtemp);
        mintempLabels[count]->setText(mintemp);
        dateLabels[count]->setAlignment(Qt::AlignCenter);
        iconLabels[count]->setAlignment(Qt::AlignCenter);
        tempLabels[count]->setAlignment(Qt::AlignCenter);
        maxtempLabels[count]->setAlignment(Qt::AlignCenter);
        mintempLabels[count]->setAlignment(Qt::AlignCenter);


QString style = R"(
    QFrame#day1, QFrame#day2, QFrame#day3, QFrame#day4,
    QFrame#day5, QFrame#day6, QFrame#day7 {
        background-color: rgba(0, 0, 0, 0.5); /* Semi-transparent black background */
        border-radius: 8px; /* Rounded corners */
    }
)";

        days[count]->setStyleSheet(style);
        count++;
    }
}


void MainWindow::on_btnReset_clicked() {
    // Clear the text fields
    ui->txtCity->clear();
    ui->txtProvince->clear();
    ui->txtPostalCode->clear();

    // Reset the radio button selection
    ui->radioBtn1Day->setChecked(true);

    // Clear the forecast display
    clearForecastDisplay();

    // Reset the background image to the default
    QPixmap background(":/images/Default.jpeg");
    background = background.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, background);
    this->setPalette(palette);

    // Reset the location info label text
    ui->lblLocationInfo->clear();

    // Reset the background color of the frames to their default
    ui->day1->setStyleSheet("background-color: rgba(0, 0, 0, 0); border-radius: 8px;");
    ui->day2->setStyleSheet("background-color: rgba(0, 0, 0, 0); border-radius: 8px;");
    ui->day3->setStyleSheet("background-color: rgba(0, 0, 0, 0); border-radius: 8px;");
    ui->day4->setStyleSheet("background-color: rgba(0, 0, 0, 0); border-radius: 8px;");
    ui->day5->setStyleSheet("background-color: rgba(0, 0, 0, 0); border-radius: 8px;");
    ui->day6->setStyleSheet("background-color: rgba(0, 0, 0, 0); border-radius: 8px;");
    ui->day7->setStyleSheet("background-color: rgba(0, 0, 0, 0); border-radius: 8px;");
}



void MainWindow::fetchCurrentLocation() {
    QString ipInfoUrl = "https://ipinfo.io/json?token=4b659d5b3268a9";  // Replace with your actual token
    QUrl url(ipInfoUrl);
    QNetworkRequest request(url);

    qDebug() << "Sending location request to IPInfo API...";
    locationNetworkManager->get(request);  // Use dedicated manager for IPInfo
}


void MainWindow::handleLocationData(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error in network reply:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
    QJsonObject jsonObj = jsonDoc.object();

    QString city = jsonObj.value("city").toString();
    QString region = jsonObj.value("region").toString();

    qDebug() << "City:" << city;
    qDebug() << "Region (Full Name):" << region;

    // Use the abbreviation if available in PROVINCE_NAME_TO_ABBREVIATION
    QString regionAbbreviation = PROVINCE_NAME_TO_ABBREVIATION.value(region, region); // Defaults to full name if not found
    qDebug() << "Province Abbreviation:" << regionAbbreviation;

    // Populate city and province fields for user confirmation but do not generate the forecast
    if (!city.isEmpty() && !regionAbbreviation.isEmpty()) {
        ui->txtCity->setText(city);
        ui->txtProvince->setText(regionAbbreviation);
    } else {
        qDebug() << "Error: Missing city or region information in JSON.";
    }

    reply->deleteLater();
}


void MainWindow::clearForecastDisplay() {
    // Widgets for displaying each day’s data (up to 7 days)
    QLabel* dateLabels[7] = {ui->Date1, ui->Date2, ui->Date3, ui->Date4, ui->Date5, ui->Date6, ui->Date7};
    QLabel* iconLabels[7] = {ui->Icon1, ui->Icon2, ui->Icon3, ui->Icon4, ui->Icon5, ui->Icon6, ui->Icon7};
    QLabel* tempLabels[7] = {ui->Temp1, ui->Temp2, ui->Temp3, ui->Temp4, ui->Temp5, ui->Temp6, ui->Temp7};
    QLabel* maxtempLabels[7] = {ui->Maxtemp1, ui->Maxtemp2, ui->Maxtemp3, ui->Maxtemp4, ui->Maxtemp5, ui->Maxtemp6, ui->Maxtemp7};
    QLabel* mintempLabels[7] = {ui->MinTemp1, ui->MinTemp2, ui->MinTemp3, ui->MinTemp4, ui->Mintemp522, ui->MinTemp6, ui->MinTemp7};
    // Clear each label
    for (int i = 0; i < 7; ++i) {
        dateLabels[i]->setText("");
        iconLabels[i]->clear();
        tempLabels[i]->setText("");
        maxtempLabels[i]->setText("");
        mintempLabels[i]->setText("");
    }
}


void MainWindow::testWeatherData() {
    clearForecastDisplay();
    QString mockApiResponse = R"({

                }
            }
        ]
    })";

    QJsonDocument jsonDoc = QJsonDocument::fromJson(mockApiResponse.toUtf8());
    QJsonObject jsonObj = jsonDoc.object();
    QJsonArray dataArray = jsonObj["data"].toArray();

    // Widgets for displaying each day’s data (up to 7 days)
    QLabel* dateLabels[7] = {ui->Date1, ui->Date2, ui->Date3, ui->Date4, ui->Date5, ui->Date6, ui->Date7};
    QLabel* iconLabels[7] = {ui->Icon1, ui->Icon2, ui->Icon3, ui->Icon4, ui->Icon5, ui->Icon6, ui->Icon7};
    QLabel* tempLabels[7] = {ui->Temp1, ui->Temp2, ui->Temp3, ui->Temp4, ui->Temp5, ui->Temp6, ui->Temp7};

    // Determine if the user selected 1 Day or 7 Days
    int days = ui->radioBtn1Day->isChecked() ? 1 : dataArray.size();
    int count = 0;

    for (int i = 0; i < days && i < 7; ++i) {  // Limit to 7 days to fit available labels
        QJsonObject dataObj = dataArray.at(i).toObject();
        QString date = dataObj["datetime"].toString();
        double temp = dataObj["temp"].toDouble();
        QJsonObject weatherObj = dataObj["weather"].toObject();
        QString iconCode = weatherObj["icon"].toString();
        QString image = ":/images/" + iconCode + ".png";
        QPixmap pic(image);

        // Check if data is properly parsed
        if (!date.isEmpty() && temp != 0) {
            dateLabels[count]->setText(date);
            iconLabels[count]->setPixmap(pic);
            tempLabels[count]->setNum(temp);
            dateLabels[count]->setAlignment(Qt::AlignCenter);
            iconLabels[count]->setAlignment(Qt::AlignCenter);
            tempLabels[count]->setAlignment(Qt::AlignCenter);
        } else {
            qDebug() << "Error: Parsed data is empty or invalid.";
        }

        count++;
    }

    // Clear any remaining labels if less than 7 days are displayed
    for (int i = count; i < 7; ++i) {
        dateLabels[i]->clear();
        iconLabels[i]->clear();
        tempLabels[i]->clear();
    }

}
