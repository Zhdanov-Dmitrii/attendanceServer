#include "server.h"

Server::Server(){}

Server::~Server(){}

void Server::startServer()
{
    if(this->listen(QHostAddress::Any, 1234))
    {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("D:\\project\\attendance\\test.db");
        if(db.open())
        {
            qDebug() << "no error";
        }
        else
        {
            qDebug() << "db is not open";
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

            QString query = "SELECT lesson.name, lesson.lecturer, GROUP_CONCAT(_group.name) as groups FROM lesson_group JOIN lesson USING(id_lesson) JOIN _group USING(id_group) GROUP BY lesson.name, lesson.lecturer ";
            query += "HAVING lesson.name like \"%";
            query += lecture;
            query += "%\" AND lesson.lecturer like \"%";
            query += teacher;
            query += "%\" AND groups like \"%";
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
                    res.append("\"teacher\":\""+queryDB->value(1).toString()+"\",");
                    res.append("\"team\":\""+queryDB->value(2).toString()+"\"},");




                }
                res.remove(res.length()-1,1);
                res.append("]}");
                socket->write(res);
                qDebug() << res;
            }
            else
            {
                qDebug() << "query is not success";
            }
        }
        else if(doc.object().value("type").toString() == "select attendance") //{"type":"select attendance", "lessonName":"", "lessonLecturer":"", "groupName":""}
        {
            QString lessonName = doc.object().value("lessonName").toString();
            QString lessonLecturer = doc.object().value("lessonLecturer").toString();
            QString groupName = doc.object().value("groupName").toString();

            QString query = "SELECT student.surname || ' ' || student.name || ' ' || student.patronymic as 'full_name', specific_lesson._date, splesson_student.status FROM splesson_student JOIN specific_lesson USING(id_specific_lesson) JOIN student USING(id_student) JOIN lesson USING(id_lesson) JOIN _group USING(id_group) JOIN lesson_group USING(id_lesson)";
            query += "WHERE lesson.name like \"%";
            query += lessonName;
            query += "%\" AND lesson.lecturer like \"%";
            query += lessonLecturer;
            query += "%\" AND _group.name like \"%";
            query += groupName;
            query += "%\"";

            qDebug() << query;

            QByteArray res = "{\"type\":\"result select attendance\", \"results\":[";

            QSqlQuery *queryDB = new QSqlQuery(db);
            if(queryDB->exec(query))
            {
                while (queryDB->next())
                {
                    res.append("{\"fio\":\""+queryDB->value(0).toString()+"\",");
                    res.append("\"date\":\""+queryDB->value(1).toString()+"\",");
                    res.append("\"+-\":\""+queryDB->value(2).toString()+"\"},");




                }
                res.remove(res.length()-1,1);
                res.append("]}");
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
