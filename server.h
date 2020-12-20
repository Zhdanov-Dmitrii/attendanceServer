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
#include <QMap>
#include <QVector>
#include <QProcess>
#include <QThread>




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
    QString queryListStudent(QString &lessonName, QString &lessonTime, QString &audit) const;
    QString queryInsertFoto(QString &lessonName, QString &lessonTime, QString &audit, QString &foto) const;
    QString queryUpdateFoto(QString &lessonName,QString &lessonTime, QString &audit, QString &foto) const;
    QString queryInsertStudentStatus(QString &lessonName,QString &lessonTime, QString &audit, QString &foto);
    QString queryInfoLesson(QString &date, QString &lessonName, QString &lecturer, QString &lessonTime);
    QString queryLogin(QString &login, QString &password);

};

#endif // SERVER_H
