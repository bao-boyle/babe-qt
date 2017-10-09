#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QMovie>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QGridLayout>
#include <fstream>
#include <iostream>

#include "notify.h"
#include "baeUtils.h"
#include "youtube.h"
#include "playlist.h"
#include "about.h"
#include "collectionDB.h"
#include "pulpo/pulpo.h"



namespace Ui {class settings;}

class TrackSaver : public QObject
{
    Q_OBJECT

public:
    TrackSaver() : QObject()
    {
        moveToThread(&t);

        qRegisterMetaType<Bae::DB>("Bae::DB");
        qRegisterMetaType<Bae::DBTables>("Bae::DBTables");

        t.start();
    }

    ~TrackSaver()
    {
        go=false;
        t.quit();
        t.wait();
    }

    void requestPath(QString path)
    {
        QMetaObject::invokeMethod(this, "getTracks", Q_ARG(QString, path));
    }

    void requestArtwork()
    {
       QMetaObject::invokeMethod(this, "fetchArtwork");
    }

    void nextTrack()
    {
        this->wait = !this->wait;
    }

    void nextAlbum()
    {
        this->wait = !this->wait;
    }


public slots:


    void fetchArtwork()
    {
        int amountArtists=0;
        int amountAlbums=0;


        QString queryTxt;
        QSqlQuery query_Covers;
        QSqlQuery query_Heads;

        connect(&connection, &CollectionDB::artworkInserted,[this](Bae::DB albumMap)
        {
            emit this->artworkReady(albumMap);
        });

        queryTxt = QString("SELECT %1, %2 FROM %3 WHERE %4 = ''").arg(Bae::DBColsMap[Bae::DBCols::ALBUM],
                Bae::DBColsMap[Bae::DBCols::ARTIST],Bae::DBTablesMap[Bae::DBTables::ALBUMS],Bae::DBColsMap[Bae::DBCols::ARTWORK]);
        query_Covers.prepare(queryTxt);

        queryTxt = QString("SELECT %1 FROM %2 WHERE %3 = ''").arg(Bae::DBColsMap[Bae::DBCols::ARTIST],
                Bae::DBTablesMap[Bae::DBTables::ARTISTS],Bae::DBColsMap[Bae::DBCols::ARTWORK]);
        query_Heads.prepare(queryTxt);

        if(query_Covers.exec())
            while (query_Covers.next())
            {
                if(go)
                {
                    QString album = query_Covers.value(Bae::DBColsMap[Bae::DBCols::ALBUM]).toString();
                    QString artist = query_Covers.value(Bae::DBColsMap[Bae::DBCols::ARTIST]).toString();
                    QString title;
                    QSqlQuery query_Title =
                            connection.getQuery("SELECT title FROM tracks WHERE artist = \""+artist+"\" AND album = \""+album+"\" LIMIT 1");
                    if(query_Title.next()) title=query_Title.value(Bae::DBColsMap[Bae::DBCols::TITLE]).toString();

                    //                connection.insertArtwork({{Bae::DBCols::ARTWORK,""},{Bae::DBCols::ALBUM,album},{Bae::DBCols::ARTIST,artist}});

                    Pulpo art({{Bae::DBCols::TITLE,title},{Bae::DBCols::ARTIST,artist},{Bae::DBCols::ALBUM,album}});

                    connect(&art, &Pulpo::albumArtReady,[&] (QByteArray array){ art.saveArt(array,Bae::CachePath); });
                    connect(&art, &Pulpo::artSaved, &connection, &CollectionDB::insertArtwork);

                    if (art.fetchAlbumInfo(Pulpo::AlbumArt,Pulpo::LastFm)) qDebug()<<"using lastfm";
                    else if(art.fetchAlbumInfo(Pulpo::AlbumArt,Pulpo::Spotify)) qDebug()<<"using spotify";
                    else if(art.fetchAlbumInfo(Pulpo::AlbumArt,Pulpo::GeniusInfo)) qDebug()<<"using genius";
                    else art.albumArtReady(QByteArray());
                    amountAlbums++;
                }
            }
        else qDebug()<<"fetchArt queryCover failed";


        if(query_Heads.exec())
            while (query_Heads.next())
            {
                if(go)
                {
                    QString artist = query_Heads.value(Bae::DBColsMap[Bae::DBCols::ARTIST]).toString();
                    Pulpo art({{Bae::DBCols::ARTIST,artist}});

                    connect(&art, &Pulpo::artistArtReady,[&] (QByteArray array){ art.saveArt(array,Bae::CachePath); });
                    connect(&art, &Pulpo::artSaved, &connection, &CollectionDB::insertArtwork);

                    art.fetchArtistInfo(Pulpo::ArtistArt,Pulpo::LastFm);

                    amountArtists++;
                }
            }
        else qDebug()<<"fetchArt queryHeads failed";


        emit this->finishedFetchingArtwork();

        Notify nof;
        nof.notify("Finished fetching art","the artwork for your collection is now ready :)\n "+QString::number(amountArtists)+" artists and "+QString::number(amountAlbums)+" albums");

    }



    void getTracks(QString path)
    {
        qDebug()<<"GETTING TRACKS FROM SETTINGS";

        QStringList urls;

        if (QFileInfo(path).isDir())
        {
            QDirIterator it(path, Bae::formats, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) urls<<it.next();
        } else if (QFileInfo(path).isFile()) urls<<path;

        emit collectionSize(urls.size());

        if(urls.size()>0)
        {
            for(auto url : urls)
            {
                if(go)
                {
                    if(!connection.check_existance(Bae::DBTablesMap[Bae::DBTables::TRACKS],Bae::DBColsMap[Bae::DBCols::URL],url))
                    {
                        TagInfo info(url);
                        QString  album = Bae::fixString(info.getAlbum());
                        int track= info.getTrack();
                        QString title = Bae::fixString(info.getTitle()); /* to fix*/
                        QString artist = Bae::fixString(info.getArtist());
                        QString genre = info.getGenre();
                        QString sourceUrl = QFileInfo(url).dir().path();
                        int duration = info.getDuration();
                        auto year = info.getYear();

                        Bae::DB trackMap
                        {
                            {Bae::DBCols::URL,url},
                            {Bae::DBCols::TRACK,QString::number(track)},
                            {Bae::DBCols::TITLE,title},
                            {Bae::DBCols::ARTIST,artist},
                            {Bae::DBCols::ALBUM,album},
                            {Bae::DBCols::DURATION,QString::number(duration)},
                            {Bae::DBCols::GENRE,genre},
                            {Bae::DBCols::SOURCES_URL,sourceUrl},
                            {Bae::DBCols::BABE, url.startsWith(Bae::YoutubeCachePath)?"1":"0"},
                            {Bae::DBCols::RELEASE_DATE,QString::number(year)}
                        };

                        emit trackReady(trackMap);
                        while(this->wait){t.msleep(100);}
                        this->wait=!this->wait;

                    }

                }else break;
            }
        }
        t.msleep(100);
        emit finished();
    }




signals:
    void trackReady(Bae::DB track);
    void artworkReady(Bae::DB album);
    void finished();
    void finishedFetchingArtwork();
    void collectionSize(int size);

private:
    QThread t;
    CollectionDB connection;
    bool go=true;
    bool wait=true;

};


class settings : public QWidget
{
    Q_OBJECT

public:

    explicit settings(QWidget *parent = nullptr);

    bool checkCollection();
    void createCollectionDB();

    int getToolbarIconSize()  {return iconSize;}
    void setToolbarIconSize(const int &iconSize);

    void setSettings(QStringList setting);
    void readSettings();
    void removeSettings(QStringList setting);
    void refreshCollectionPaths();
    void collectionWatcher();
    void addToWatcher(QStringList paths);
    QStringList getCollectionPath() {return collectionPaths;}
    CollectionDB collection_db;
    bool youtubeTrackDone=false;

    enum iconSizes
    {
        s16,s22,s24
    };
    //enum albums { ALBUM_TITLE, ARTIST, ART};
    // enum artists { ARTIST_TITLE, ART};

private slots:

    void on_open_clicked();
    void on_toolbarIconSize_activated(const QString &arg1);
    void on_pushButton_clicked();
    void handleDirectoryChanged(const QString &dir);
    void on_collectionPath_clicked(const QModelIndex &index);
    void on_remove_clicked();

    void on_debugBtn_clicked();
    void on_checkBox_stateChanged(int arg1);

public slots:

    void populateDB(const QString &path);
    void fetchArt();
    void handleDirectoryChanged_extension();


private:
    Ui::settings *ui;
    TrackSaver trackSaver;
    bool busy =false;

    const QString notifyDir= Bae::NotifyDir;
    const QString collectionDBName = "collection.db";
    const QString settingsName = "settings.conf";

    Notify nof;
    YouTube *ytFetch;

    int iconSize = 16;
    QStringList collectionPaths={};
    QLabel *artFetcherNotice;
    QMovie *movie;
    QString pathToRemove;
    // QFileSystemWatcher watcher;
    //QThread* thread;
    About *about_ui;
    QStringList files;
    QStringList dirs;
    QFileSystemWatcher *watcher;
    QFileSystemWatcher *extensionWatcher;
    QTimer *cacheTimer;

signals:

    void toolbarIconSizeChanged(int newSize);
    void collectionPathChanged(QString newPath);
    void collectionDBFinishedAdding();
    void refreshTables(const Bae::DBTables &reset);
    void finishedTrackInsertion();
    void getArtwork();
    void albumArtReady(const Bae::DB &albumMap);

};

#endif // SETTINGS_H
