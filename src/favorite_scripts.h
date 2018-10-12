#ifndef FAVORITE_SCRIPTS_H
#define FAVORITE_SCRIPTS_H
#include <QIcon>
#include <QList>
#include <QString>

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
    bool save_to_file(QList<ScriptEntry> script_entries);
    const QList<ScriptEntry> get_favorite_scritps() const;

    ScriptEntry get_entry(QString script_path);

    bool add_favorite(QString script_path);
    bool set_alternative_name(QString script_path, QString alternative_name);
    bool remove_favorite(QString script_path);
    bool is_favorite(QString script_path);

    private:
    QString file_name_m;
    QList<ScriptEntry> script_entries_m;
    int get_index_by_path(QString script_path);
};

#endif // FAVORITE_SCRIPTS_H
