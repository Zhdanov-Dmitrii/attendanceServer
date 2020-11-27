#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonParseError>

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

    QJsonDocument doc;
    QJsonParseError docError;
};

#endif // SERVER_H
