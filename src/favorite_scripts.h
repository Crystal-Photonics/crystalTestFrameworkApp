#ifndef FAVORITE_SCRIPTS_H
#define FAVORITE_SCRIPTS_H
#include <QList>
#include <QString>
#include <QIcon>

typedef struct {
    bool valid;
    QString script_path;
    QString alternative_name;
    QString description;
    QIcon icon;
} ScriptEntry;

class FavoriteScripts {
    public:
    FavoriteScripts();
    void load_from_file(QString file_name);
    void save_to_file();
    const QList<ScriptEntry> get_favorite_scritps() const;

    ScriptEntry get_entry(QString script_path);

    void add_favorite(QString script_path);
    void set_alternative_name(QString script_path, QString alternative_name);
    void remove_favorite(QString script_path);
    bool is_favorite(QString script_path);

    private:
    QString file_name_m;
    QList<ScriptEntry> script_entries_m;
    int get_index_by_path(QString script_path);
};

#endif // FAVORITE_SCRIPTS_H
