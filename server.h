#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDate>
#include <QFile>

class Server : public QTcpServer
{
    Q_OBJECT
public:
    Server();
    ~Server();

public slots:
    void startServer();
    void incomingConnection(qintptr socketDescriptor) override;
    void sockReady();
    void sockDisc();


private:  
    QMap< qintptr, QTcpSocket*> listSocket;
    QByteArray data;
    QSqlDatabase db;

    QJsonDocument doc;
    QJsonParseError docError;


    QString queryLesson(QString &lecture, QString &teacher, QString team) const;
    QString queryAttendance(QString &groupName, QString &lessonLecturer, QString &lessonName) const;
    QString queryUpdateStudentStatus(QString &fio, QString &lessonName, QString &lessonTime, QString &date, QString &status) const;
};

#endif // SERVER_H
