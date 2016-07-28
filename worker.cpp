#include "worker.h"
#include <QFile>

worker::worker(QObject *parent) :
    QObject(parent)
{
}

void worker::Scan(){

    QFile file(pathSave);
    file.remove();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        listError.append("Не удалось создать или открыть файл");
        emit onComplit();
        return;
    }

    if(!db->exec("create table if not exists skus_temp(id int auto_increment primary key,name varchar(100), "
                 "prise varchar(100),count int,prod_id int) DEFAULT CHARACTER SET cp1251 COLLATE cp1251_general_ci;"
                 " ALTER TABLE skus_temp ADD UNIQUE INDEX in_id (id ASC); "
                 " ALTER TABLE skus_temp ADD INDEX in_prod (prod_id ASC); "
                 ))
        {
            listError.append("Не удалось выполнить запрос create table skus_temp" + db->lastError);
            emit onComplit();
            return;
        }
        if(!db->exec("truncate table skus_temp;"))
        {
            listError.append("Не удалось выполнить запрос truncate table skus_temp;");
            emit onComplit();
            return;
        }
    db->exec("alter table product add art nvarchar(100);");

    if(!db->query->exec("SET SQL_SAFE_UPDATES = 0;")){
        listError.append("Не удалось выполнить запрос SET SQL_SAFE_UPDATES = 0;");
        emit onComplit();
        return;
    }
    if(!db->query->exec("update product set art = concat('1',LPAD(id,5,'0')) where art is null ;")){
        listError.append("Не удалось выполнить запрос update proc");
        emit onComplit();
        return;
    }

    foreach (string str, sl) {
        QStringList s = str.split(",");
        string art="",pr="",c="";
        if(s.size() > 4){
            listError.append("Запятых больше 4х");
            emit onComplit();
            return;
        }
        if(s.size() == 4){
            art = s[0];
            pr = string(s[1]) +string(",") +string(s[2]);
            c = s[3];
        }

        if(s.size() == 3){
            art = s[0];
            pr = s[1];
            c = s[2];
        }
        if(s.size() == 2){
            art = s[0];
            pr = s[1];
        }
        if(s.size() == 1){
            art = s[0];
        }
        db->query->exec(string("insert into skus_temp(name,prise,count) values('%1','%2',%3)").arg(art,pr,c));
    }

    string temp = "create temporary table if not exists temp_data(id int,har_value nvarchar(500),name nvarchar(500),prod nvarchar(500),art nvarchar(255),image nvarchar(255),brand nvarchar(255)) "
            " DEFAULT CHARACTER SET cp1251 COLLATE cp1251_general_ci; "
            " ALTER TABLE temp_data ADD INDEX in_prod_id (id ASC); "
            " ALTER TABLE temp_data ADD INDEX in_name (`name` ASC); "
            " truncate table temp_data; "
            " insert into temp_data(id,har_value,name,prod,art,image,brand)  "
            " select p.id,v.har_value,h.name,p.name prod,p.art,p.image,p.brand from hars_values v join hars h on v.har_id = h.id    "
                                  " join product p on v.prod_id=p.id and h.cat_id = p.cat_id ;";
    if(!db->query->exec(temp)){
        listError.append("Не удалось создать временную таблицу temp_data");
        emit onComplit();
        return;
    }

    //обновление продуктов
    string qup ="update skus_temp s "
            "join (SELECT har_value,id FROM  temp_data where name = 'Артикул') v1 on v1.har_value = s.name "
              " set prod_id = v1.id "
      " where s.id>0; ";

     if(!db->query->exec(qup)){
         listError.append("Не удалось выполнить запрос update skus_temp s");
         emit onComplit();
         return;
     }


     if(!db->query->exec("SET global group_concat_max_len = 18446744073709551615;")){
              listError.append("Не удалось выполнить SET global group_concat_max_len =");
              emit onComplit();
              return;
          }
     string q = "select t1.id,replace(t1.prod,'#233;','') prod,t1.artic,t1.art,t1.prise,t1.count,t1.xm,t1.htm,t1.brand,"
                "      concat(t1.image,ifnull(concat(';',GROUP_CONCAT(i.name SEPARATOR ';')),'') ) as i  "
                " from ( "
                " SELECT dt.id,dt.prod,st.name artic,dt.art,st.prise,st.count,dt.image,dt.brand,"
                " GROUP_CONCAT(concat('<li><b>',replace(dt.name,'&#730;',''),'</b> : ',dt.har_value) SEPARATOR  '</li>')  as xm,  "
                " GROUP_CONCAT(concat('<tr><td>',replace(dt.name,'&#730;',''),'</td><td>',dt.har_value,'</td>') SEPARATOR '</tr>')  as htm "
                " FROM (select id,har_value,name,prod,art,image,brand from temp_data where name <> 'Артикул' ) as dt  "
                "                       right join skus_temp st on st.prod_id = dt.id "
                " group by dt.id,dt.prod,st.name,dt.art,st.prise,st.count,dt.image,dt.brand) t1 left join images i on t1.id = i.prod_id  "
                " group by t1.id,t1.prod,t1.artic,t1.art,t1.prise,t1.count,t1.xm,t1.htm,t1.image,t1.brand"
                " order by t1.id;";

    if(!db->exec(q)){
        listError.append("Не удалось выполнить запрос выборки всех данных - " + db->lastError);
         emit onComplit();
        return;
    }
    int i = 0;


    QTextStream out(&file);
    out.setCodec("windows-1251");
    out  <<QString("Наименование;")<<
           QString("Наименование артикула;")<<
           QString("Артикул;")<<
           QString("Валюта;")<<
           QString("Цена;")<<
           QString("Доступен для заказа;")<<
           QString("Зачеркнутая цена;")<<
           QString("Закупочная цена;")<<
           QString("В наличии;")<<
           QString("Краткое описание;")<<
           QString("Описание;")<<
           QString("Наклейка;")<<
           QString("Статус;")<<
           QString("Тип товаров;")<<
           QString("Бренд;")<<
           QString("Теги;")<<
           QString("Облагается налогом;")<<
           QString("Заголовок;")<<
           QString("META Keywords;")<<
           QString("META Description;")<<
           QString("Ссылка на витрину;")<<
           QString("Дополнительные параметры");
            for (int k = 0; k < 65; ++k) {
                out << QString(";Изображения");
            }
            out << QString("\n");

            QString n = QString(";");
            while(db->query->next()){
                //qDebug() << "1111";
                //out  << db->query->value(0).toString() << n;//id

                if(db->query->value(0).isNull()){
                    listNot.append(db->query->value(2).toString() +","+
                                        db->query->value(4).toString() +","+
                                        db->query->value(5).toString()
                                           );
                    continue;
                }
                out  << db->query->value(1).toString() << n;///prod "Наименование"
                out  << db->query->value(2).toString() << n;/// art "Артикула"
                out  << db->query->value(3).toString() << n;///artic "Наименование артикул"
                out  << "RUB;";                                                        ///"Валюта"
                out  << db->query->value(4).toString() << n;/// prise "Цена"
                out  << "1;";///"Доступен для заказа")<<
                out  << "0;";///"Зачеркнутая цена")<<
                out  << "0;";///"Закупочная цена"
                out  << db->query->value(5).toString() << n;/// count "В наличии"
                out  << QString("<ul>") << db->query->value(6).toString()
                     << QString("</li><li><b>")<< QString("Бренд</b>: ") << db->query->value(8).toString()
                     << QString("</li>") <<QString("</ul>") << n;/// xm "Краткое описание"
                out  << QString("<table>") << db->query->value(7).toString()
                     << QString("</tr><tr><td>")<< QString("Бренд</td><td>") << db->query->value(8).toString()
                     << QString("</td></tr>")
                     << QString("</table>") << n;/// htm "Описание"
                out  << n;/// "Наклейка")<<
                out  << QString("1;");/// "Статус")<<
                out  << n;/// "Тип товаров")<<
                out  << n; //db->query->value(8).toString()<< n;/// "Бренд")<< //специально оставляем пустым, чтобы на сайте отображалось корректно
                out  << n;/// "Теги")<<
                out  << n;/// "Облагается налогом")<<
                out  << db->query->value(1).toString() << n;///prod "Заголовок")<<
                out  << db->query->value(1).toString() << n;///prod "META Keywords")<<
                out  << db->query->value(1).toString() << n;///prod "META Description")<<
                out  << n;/// "Ссылка на витрину")<<
                out  << n;/// "Дополнительные параметры" << n;
                out  << db->query->value(9).toString();/// img(;)
                out  << QString("\n");
            }
            file.close();

            emit onComplit();
}