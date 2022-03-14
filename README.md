# MySQL2Other

## 项目介绍：

采用C++编写，运行于Linux环境，采用Docker搭建测试环境，MySQL5.6和Redis数据库，暴露相应端口号进行测试。

主要内容：根据配置文件，链接相应MySQL，根据模板工厂生成其他数据库对象实例。以MySQL和Redis为例。根据MySQL主从同步原理，MySQL_Connector实例伪装成Slaver，连接MySQL Master，向Master 发送同步请求，随后拉取对应Binlog文件进行内容解析，将解析结果根据配置文件筛选出保留内容，随后将解析结果分发到其他数据库实例阻塞队列中，根据Redis键值对配置，采用线程池将数据再次处理后向数据库实例发送数据。程序最高兼容MySQL5.6版本



## 环境介绍：

配置文件采用yaml格式编写，具体内容如下：

```
mysql:
  connect:
    mysql_ip: 172.21.16.1
    mysql_port: 3308
    username: root
    password: 456123az
    need_decode: false
    server_id: 1
  filter:
    - database: mysql2other
      table: student
      reserve_column_name:
        - index: 1
          name: Sno
        - index: 2
          name: Sname
        - index: 3
          name: Ssex
redis:
  connect:
    redis_ip: 172.21.16.1
    redis_port: 6379
    need_passwd: false
    password: 0
  import:
    - db_table: mysql2other_student
      key_prefix: student_
      key_name: Sno
      value:
        - Sname
        - Ssex
```

测试环境搭建：

```
Docker:
docker run  -d -p 3308:3306 -e MYSQL_ROOT_PASSWORD=456123az -v mysql_conf:/etc/mysql/ -v mysql_data:/var/lib/mysql --name mysqlDB mysql:5.6
将mysql日志存储格式改为row存储
配置文件中：
binlog_format=ROW
log-bin=mysql-bin

docker  run -d -p 6379:6379 --name redis redis
```



## 具体架构：

日志模块采用Google 开源日志库Glog

配置读取模块采用开源库 Yaml-cpp

连接数据库分别采用 mysqlclient 和 hiredis 库

连接过程中的加解密采用ssl 与 crypto库

设计分为如下模块：

**LineData模块**：存储解析出来的数据，存储内容为column_name , old_value , new_value

**配置模块**：采用单例设计，对于每个连接的数据库，设置两个数据结构，connect_config 和 filter_config分别用于连接对应数据库，与对数据进行处理；

**解析模块**：根据Mysql连接完成后获取的fd，拉取对应数据，每次拉取一个packet，随后对该packet进行解析,MySQL packet 分为正常包和错误包，根据状态码调用相应的解析类。正常包可以继续细分为如下事件类型：

| Event                                      | description                                                  |
| ------------------------------------------ | ------------------------------------------------------------ |
| Binlog Event header                        | 记录了本次请求的binlog文件名和位置,由master进程生成，实际的binlog中并不存在 |
| Format_Description_Event                   | 描述了binlog版本和服务器版本，以及文件中各个event类型的post header 长度。 |
| Table_Map_Event                            | 描述了将要发生更新的表的表结构                               |
| (Write\|Update\|Delete)_Rows_Event(v1\|v2) | 描述了增删改事件，需要与Table_Map_Event配合进行解析          |
| Rotate_Event(binlog文件发生切换)           | 如果binlog发生切换，这时会在当前使用的binlog文件末尾添加一个ROTATE_EVENT事件，将下一个binlog文件的名称和位置记录到该事件中。 |

也有其他类型的事件，但是对于解析有用的只有这些。详情在官网[MySQL :: MySQL Internals Manual :: 14.9.4.6 FORMAT_DESCRIPTION_EVENT](https://dev.mysql.com/doc/internals/en/format-description-event.html)

随后根据事件类型调用对应解析类。最终将解析数据存入LineData类中

**过滤器模块**：根据配置文件生成的单例对象，针对LineData数据进行过滤，保留所需要的列的数据。

**其他数据库模块**：定义抽象基类ImportBase，采用模板工厂模式，将数据库类注册到工厂中，随后调用工厂类生成对应数据库连接实例。

**线程池模块**：单例对象，以function<void()>的形式，存储指定任务，内置阻塞队列，对外提供存放任务的接口，每当接收到一个任务就循环执行。以条件变量的形式进行线程同步

**事件分发器模块**：将数据库连接实例以ImportBase指针形式存入，负责将过滤后的LineData分发到其他数据库实例中的阻塞队列，将对应解析和发送LineData的任务放到线程池模块中，将具体的发送任务交给线程池



## 数据流程：

fd --> Packet --> Event --> LineData --> Filter --> LineData --> dispatcher -->  OtherDB_LineData_Queue --> ThreadPool --> SendData



具体事件类型解析

| Event                                      | description                                                  |
| ------------------------------------------ | ------------------------------------------------------------ |
| Binlog Event header                        | 记录了本次请求的binlog文件名和位置,由master进程生成，实际的binlog中并不存在 |
| Format_Description_Event                   | 描述了binlog版本和服务器版本，以及文件中各个event类型的post header 长度。 |
| Table_Map_Event                            | 描述了将要发生更新的表的表结构                               |
| (Write\|Update\|Delete)_Rows_Event(v1\|v2) | 描述了增删改事件，需要与Table_Map_Event配合进行解析          |
| Rotate_Event(binlog文件发生切换)           | 如果binlog发生切换，这时会在当前使用的binlog文件末尾添加一个ROTATE_EVENT事件，将下一个binlog文件的名称和位置记录到该事件中。 |

1. ```
   Binlog Event header
   //The binlog event header starts each event and is either 13 or 19 bytes long, depending on the binlog version.
   
   Payload:
   4              timestamp
   1              event type
   4              server-id
   4              event-size
      if binlog-version > 1:
   4              log pos
   2              flags
   ```

2. ```
   FORMAT_DESCRIPTION_EVENT
   //A format description event is the first event of a binlog for binlog-version 4. It describes how the other events are layed out.
   
   Payload:
   2                binlog-version
   string[50]       mysql-server version
   4                create timestamp
   1                event header length
   string[p]        event type header lengths
   ```

3. ```
   TABLE_MAP_EVENT:
   //The first event used in Row Based Replication declares how a table that is about to be changed is defined.
   
   post-header:
       if post_header_len == 6 {
     4              table id
       } else {
     6              table id
       }
     2              flags
   
   payload:
     1              schema name length
     string         schema name
     1              [00]
     1              table name length
     string         table name
     1              [00]
     lenenc-int     column-count
     string.var_len [length=$column-count] column-def
     lenenc-str     column-meta-def
     n              NULL-bitmask, length: (column-count + 8) / 7
   ```

4. ```
   ROWS_EVENT
   //Version 1
   written from MySQL 5.1.15 to 5.6.x
   	UPDATE_ROWS_EVENTv1
   	WRITE_ROWS_EVENTv1
   	DELETE_ROWS_EVENTv1
   added the after-image for the UPDATE_ROWS_EVENT
   Version 2
   written from MySQL 5.6.x
   	UPDATE_ROWS_EVENTv2
   	WRITE_ROWS_EVENTv2
   	DELETE_ROWS_EVENTv2
   added the extra-data fields
   
   header:
     if post_header_len == 6 {
   4                    table id
     } else {
   6                    table id
     }
   2                    flags
     if version == 2 {
   2                    extra-data-length
   string.var_len       extra-data
     }
   
   body:
   lenenc_int           number of columns
   string.var_len       columns-present-bitmap1, length: (num of columns+7)/8
     if UPDATE_ROWS_EVENTv1 or v2 {
   string.var_len       columns-present-bitmap2, length: (num of columns+7)/8
     }
   
   rows:
   string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap1'+7)/8
   string.var_len       value of each field as defined in table-map
     if UPDATE_ROWS_EVENTv1 or v2 {
   string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap2'+7)/8
   string.var_len       value of each field as defined in table-map
     }
     ... repeat rows until event-end
   ```

5. ```
   Rotate_Event
   //The rotate event is added to the binlog as last event to tell the reader what binlog to request next.
   
   Post-header:
     if binlog-version > 1 {
   8              position
     }
   Payload:
   
   string[p]      name of the next binlog
   
   ```

