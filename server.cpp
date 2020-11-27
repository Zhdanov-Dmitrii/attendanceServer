#include "server.h"

Server::Server(){}

Server::~Server(){}

void Server::startServer()
{
    if(this->listen(QHostAddress::Any, 1234))
    {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("D:\\project\\attendance\\temp.db");
        if(db.open())
        {
            qDebug() << "no error";
        }
    }
    else
    {
    }
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);


    listSocket[socketDescriptor] = socket;

    connect(socket, SIGNAL(readyRead()),this,SLOT(sockReady()));
    connect(socket, SIGNAL(disconnected()),this,SLOT(sockDisc()));

    qDebug()<<  socketDescriptor << " connected";

    qDebug()<<socket->write("you are connected");
    qDebug() << "Send client connect status - yes";
}

void Server::sockReady()
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    QByteArray data = socket->readAll();

    doc = QJsonDocument::fromJson(data, &docError);
    if(docError.errorString().toUInt() == QJsonParseError::NoError)
    {
        //{"type":"select lecture", "lecture":"lecture", "teacher":"teacher", "team":"team"}
        if(doc.object().value("type").toString() == "select lecture")
        {
            QString lecture = doc.object().value("lecture").toString();
            QString teacher = doc.object().value("teacher").toString();
            QString team = doc.object().value("team").toString();

            QString query = "SELECT lecture.name, lecture.teacher, team.name FROM (lecture JOIN team_lecture USING(id_lecture)) JOIN team USING(id_team) ";
            query += "WHERE lecture.name like \"%";
            query += lecture;
            query += "%\" AND lecture.teacher like \"%";
            query += teacher;
            query += "%\" AND team.name like \"%";
            query += team;
            query += "%\"";

            qDebug() << query;

            QByteArray res = "{\"type\":\"result select lecture\", \"results\":[";

            QSqlQuery *queryDB = new QSqlQuery(db);
            if(queryDB->exec(query))
            {
                while (queryDB->next())
                {
                    res.append("{\"lecture\":\""+queryDB->value(0).toString()+"\",");
                    res.append("{\"teacher\":\""+queryDB->value(1).toString()+"\",");
                    res.append("{\"team\":\""+queryDB->value(2).toString()+"\",");

                    res.remove(res.length()-1,1);
                    res.append("]}");


                }
                socket->write(res);
                qDebug() << res;
            }
            else
            {
                qDebug() << "query is not success";
            }




        }
    }
}

void Server::sockDisc()
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    qDebug()<<"disconnect ";
    socket->deleteLater();
}
