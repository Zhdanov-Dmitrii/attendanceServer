#include "server.h"

Server::Server(){}

Server::~Server(){}

void Server::startServer()
{
    if(this->listen(QHostAddress::Any, 1234))
    {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("D:\\project\\attendance\\attendanceServer\\test.db");
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
    query += "%') GROUP BY name, groups, lecturer;";

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
    QDate d = QDate::fromString(date,"dd.MM.yyyy");

    QString query ="UPDATE splesson_student SET status = '";
    query += status;
    query += "' WHERE id_student = (SELECT id_student FROM student JOIN _group USING(id_group) WHERE student.surname || ' ' || student.name || ' ' || student.patronymic || ' (' || _group.name || ')' like '%";
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

QString Server::queryListStudent(QString &lessonName, QString &lessonTime, QString &audit)
{
    QDate d = QDate::currentDate();


    QString query ="SELECT student.surname || ' ' || student.name || ' ' || student.patronymic, _group.name, student.photo FROM lesson JOIN lesson_group USING(id_lesson) JOIN _group USING(id_group) JOIN student USING(id_group) WHERE lesson._time like '%";
    query += lessonTime;
    query += "%' AND lesson.auditorium like '%";
    query += audit;
    query += "%' AND lesson.name like '%";
    query += lessonName;
    query += "%' AND lesson.day_of_week like '%1";
    //query += QString::number(d.dayOfWeek());
    query += "%'";

    return query;
}



void Server::sockReady()
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    QByteArray data = socket->readAll();



    QByteArray dataFoto = data;

    dataFoto.remove(0, data.indexOf('}')+1);
    data.remove(data.indexOf('}')+1, data.size()-data.indexOf('}'));
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
                qDebug() << "query is success";
            }
            else
            {
                qDebug() << "query is not success";
                qDebug() << query;
            }
            delete queryDB;
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
                qDebug() << "query is success";
            }
            else
            {
                qDebug() << "query is not success";
                qDebug() << query;
            }
            delete queryDB;
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
            delete queryDB;
        }
        else if(doc.object().value("type").toString() == "face recognition")//{"type":"face recognition", "data":"
        {
            QFile foto("D:\\project\\attendance\\build-attendanceServer-Desktop_x86_windows_msys_pe_64bit-Debug\\debug\\foto.jpg");
            if(!foto.open(QIODevice::WriteOnly))
            {
                qDebug() <<"file is not open";
                return;
            }
            foto.write(dataFoto);
            foto.close();

            QString lessonName = doc.object().value("lesson name").toString();
            QString time = doc.object().value("time").toString();
            QString audit = doc.object().value("audit").toString();

            QString query = queryListStudent(lessonName, time, audit);
            QSqlQuery* queryBD = new QSqlQuery(db);


            QVector<QString> name;
            QVector<QString> group;
            QMap<QString, int> listStudent;
            QVector<QVector<int>> locate;

            //выполнение запроса
            int k = 0;
            if(queryBD->exec(query))
            {
                while (queryBD->next())
                {
                    name.push_back(queryBD->value(0).toString());
                    group.push_back(queryBD->value(1).toString());
                    listStudent.insert(queryBD->value(2).toString(), k);
                    qDebug() << queryBD->value(2).toString();
                    k++;
                }
            }


            //открытие py-скрипта
            QProcess *qprocess = new QProcess(this);
            qprocess->start("cmd");

            if(!qprocess->waitForStarted())
                qDebug() << "cmd is not open";

            QString pyCommand("python D:\\project\\attendance\\build-attendanceServer-Desktop_x86_windows_msys_pe_64bit-Debug\\main.py \n"); //try with out " \n" also...
            qprocess->write(pyCommand.toLatin1().data());

            qprocess->write(QString::number(k).toLatin1()+'\n');
            for(auto it = listStudent.begin(); it != listStudent.end(); it++)
            {
                qprocess->write(it.key().toLatin1()+'\n');
                qprocess->waitForBytesWritten();
            }
            qprocess->write("D:\\project\\attendance\\build-attendanceServer-Desktop_x86_windows_msys_pe_64bit-Debug\\debug\\foto.jpg\n");
            qprocess->waitForBytesWritten();
            if(!qprocess->waitForReadyRead(30000))
            {
                qDebug() << "not read";
                return;
            }



            //
            QByteArray res = "{\"type\":\"result face recognition\", \"results\":[";

            QFile fres("D:\\project\\attendance\\build-attendanceServer-Desktop_x86_windows_msys_pe_64bit-Debug\\debug\\output.txt");
            while(!fres.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QThread::sleep(3);
            }
            k = fres.readLine().toInt();

            for(int i = 0; i < k; i++)
            {
                QString t = fres.readLine();
                QStringList tt = t.split(' ');
                qDebug() << name[listStudent[tt[0]]];
                res += "{\"name\":\"" + name[listStudent[tt[0]]] + "\", \"group\":\"" + group[listStudent[tt[0]]]+"\", \"status\":\"+\"},";
                listStudent.remove(tt[0]);
            }

            fres.close();
            fres.remove();

            for(auto it = listStudent.begin(); it != listStudent.end(); it++)
                res += "{\"name\":\"" + name[it.value()] + "\", \"group\":\"" + group[it.value()]+"\", \"status\":\"-\"},";

            res.remove(res.size()-1,1);
            res += "]}";

            socket->write(res);




            delete queryBD;
        }
    }
}

void Server::sockDisc()
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    qDebug()<<"disconnect ";
    socket->deleteLater();
}
/*
вставка фото в бд
{
QDate d = QDate::fromString(date,"dd.MM.yyyy");


QString query ="INSERT INTO specific_lesson (id_lesson) SELECT lesson.id_lesson FROM lesson WHERE lesson.name like '%";
query += lessonName;
query += "%' AND lesson._time like '%";
query += lessonTime;
query += "%' AND lesson.auditorium like '%";
query += ; //аудитория
query += "%' AND lesson.day_of_week like '%";
query += QString::number(d.dayOfWeek());
query += "%'; UPDATE specific_lesson SET photo = '";
query += ; //фото с пары
query += "', _date = strftime('%d.%m.%Y','now') WHERE _date is NULL"

return query;
}

добавление автоматического минуса в статусе
{
QDate d = QDate::fromString(date,"dd.MM.yyyy");


QString query ="INSERT INTO splesson_student (id_specific_lesson) SELECT specific_lesson.id_specific_lesson FROM specific_lesson JOIN lesson USING(id_lesson) WHERE lesson.name like '%";
query += lessonName;
query += "%' AND lesson._time like '%";
query += lessonTime;
query += "%' AND lesson.auditorium like '%";
query += ; //аудитория
query += "%' AND lesson.day_of_week like '%";
query += QString::number(d.dayOfWeek());
query += "%' AND specific_lesson._date = strftime('%d.%m.%Y','now'); UPDATE splesson_student SET id_student = (SELECT id_student FROM student WHERE student.photo like '%";
query += ; //фото студента
query += "%' ) WHERE id_student is NULL;"

return query;
}


изменение статуса по фото

{
QDate d = QDate::fromString(date,"dd.MM.yyyy");


QString query ="UPDATE splesson_student SET status = '";
query += status;
query += "' WHERE id_student = (SELECT id_student FROM student WHERE student.photo like '%";
query += ; //адрес фото
query += "%') AND id_specific_lesson = (SELECT specific_lesson.id_specific_lesson FROM specific_lesson JOIN lesson USING(id_lesson) WHERE lesson.name like '%";
query += lessonName;
query += "%' AND lesson._time like '%";
query += lessonTime;
query += "%' AND lesson.day_of_week like '%";
query += QString::number(d.dayOfWeek());
query += "%' AND specific_lesson._date = strftime('%d.%m.%Y','now') AND lesson.auditorium like '%"
query += ; //аудитория
query += "%')";

return query;
}
*/
