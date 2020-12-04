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

QString Server::queryLesson(QString &lecture, QString &teacher, QString team) const
{
    QString query = "SELECT name, lecturer, groups FROM (SELECT lesson.name as name, lesson.lecturer as lecturer, GROUP_CONCAT(_group.name) as groups FROM lesson_group JOIN lesson USING(id_lesson) JOIN _group USING(id_group) GROUP BY lesson.id_lesson ";
    query += "HAVING lesson.name like '%";
    query += lecture;
    query += "%' AND lesson.lecturer like '%";
    query += teacher;
    query += "%' AND groups like '%";
    query += team;
    query += "%') GROUP BY name, groups, lecturer";

    return query;
}

QString Server::queryAttendance(QString &groupName, QString &lessonLecturer, QString &lessonName) const
{
    QString query = "SELECT student.surname || ' ' || student.name || ' ' || student.patronymic || ' (' || _group.name || ')' as full_name, specific_lesson._date || ' (' || lesson._time || ')' as date, splesson_student.status ";
    query += "FROM splesson_student JOIN specific_lesson USING(id_specific_lesson) JOIN student USING(id_student) JOIN lesson USING(id_lesson) JOIN _group USING(id_group) JOIN lesson_group USING(id_lesson) ";

    query += "WHERE EXISTS (SELECT GROUP_CONCAT(_group.name) as groups FROM lesson_group JOIN lesson USING(id_lesson) JOIN _group USING(id_group) GROUP BY lesson.id_lesson HAVING groups like '%";
    query += groupName;
    query += "%') AND lesson.name like '%";
    query += lessonName;
    query += "%' AND lesson.lecturer like '%";
    query += lessonLecturer;
    query += "%' GROUP BY student.id_student, specific_lesson.id_specific_lesson ORDER BY date";

    return query;
}

QString Server::queryUpdateStudentStatus(QString &fio, QString &lessonName, QString &lessonTime, QString &date, QString &status) const
{
    QDate d;
    d.fromString(date,"dd.MM.yyyy");


    QString query ="UPDATE splesson_student SET status = '";
    query += status;
    query += "' WHERE id_student = (SELECT id_student FROM student WHERE student.surname || ' ' || student.name || ' ' || student.patronymic like '%";
    query += fio;
    query += "%') AND id_specific_lesson = (SELECT specific_lesson.id_specific_lesson FROM specific_lesson JOIN lesson USING(id_lesson) WHERE lesson.name like '%";
    query += lessonName;
    query += "%' AND lesson._time like '%";
    query += lessonTime;
    query += "%' AND lesson.day_of_week like '%";
    query += QString::number(d.dayOfWeek());
    query += "%' AND specific_lesson._date like '%";
    query += date;
    query += "%');";

    return query;
}

void Server::sockReady()
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    QByteArray data = socket->readLine();
    qDebug() << data;
    doc = QJsonDocument::fromJson(data, &docError);
    if(docError.errorString().toUInt() == QJsonParseError::NoError)
    {
        qDebug() << doc.object().value("type").toString();
        //{"type":"select lecture", "lecture":"lecture", "teacher":"teacher", "team":"team"}
        if(doc.object().value("type").toString() == "select lecture")
        {
            QString lecture = doc.object().value("lecture").toString();
            QString teacher = doc.object().value("teacher").toString();
            QString team = doc.object().value("team").toString();

            //qDebug() << query;

            QByteArray res = "{\"type\":\"result select lecture\", \"results\":[";

            QString query = queryLesson(lecture, teacher, team);
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
            }
            else
            {
                qDebug() << "query is not success";
                qDebug() << query;
            }
        }
        else if(doc.object().value("type").toString() == "select attendance") //{"type":"select attendance", "lessonName":"", "lessonLecturer":"", "groupName":""}
        {
            QString lessonName = doc.object().value("lessonName").toString();
            QString lessonLecturer = doc.object().value("lessonLecturer").toString();
            QString groupName = doc.object().value("groupName").toString();


           // qDebug() << query;

            QByteArray res = "{\"type\":\"result select attendance\", \"results\":[";

            QString query = queryAttendance(groupName, lessonLecturer, lessonName);
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
                qDebug() << query;
            }

        }
        else if(doc.object().value("type").toString() == "update student status")
        {
            QString fio, lessonName, lessonTime, date, status;
            fio = doc.object().value("fio").toString();
            lessonName = doc.object().value("lessonName").toString();
            lessonTime = doc.object().value("lessonTime").toString();
            date = doc.object().value("date").toString();
            status = doc.object().value("status").toString();
            QString query = queryUpdateStudentStatus(fio, lessonName, lessonTime, date, status);
            QSqlQuery *queryDB = new QSqlQuery(db);
            if(queryDB->exec(query))
                qDebug() << "query is success";
            else
            {
                qDebug() << "query is not success";
                qDebug() << query;
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
