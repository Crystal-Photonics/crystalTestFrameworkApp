#ifndef DATA_ENGINE_STRINGS_H
#define DATA_ENGINE_STRINGS_H


const QString QUERY_CREATE_INSTANCE_TABLE  = QString{R"(
                                            CREATE TABLE %1 (
                                                InstanceID int PRIMARY KEY,
                                                Caption text
                                            )
                                        )"};



const QString QUERY_CREATE_DATA_TABLE = QString{R"(
            CREATE TABLE %1 (
                ID int PRIMARY KEY,
                Name text,
                Description text,
                Desired text,
                Actual text,
                InstanceID int KEY,
                Inrange text,
                Unit text
            )
        )"};
#endif // DATA_ENGINE_STRINGS_H
