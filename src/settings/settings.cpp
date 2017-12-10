/*
   Babe - tiny music player
   Copyright (C) 2017  Camilo Higuita
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

   */


#include "settings.h"
#include "ui_settings.h"
#include "../services/local/socket.h"

settings::settings(QWidget *parent) : QWidget(parent), ui(new Ui::settings)
{
    ui->setupUi(this);

    // QFrame frame = new QFrame();

    // collectionDBPath=QDir().absolutePath()+collectionDBPath;

    /*LOAD SAVED SETTINGS*/

    this->ui->pulpoBrainz_checkBox->setChecked(Bae::loadSettings("BRAINZ","SETTINGS",true).toBool());
    this->ui->pulpoBrainz_spinBox->setValue(Bae::loadSettings("BRAINZ_INTERVAL","SETTINGS",15).toInt());

    qDebug() << "Getting collectionDB info from: " << Bae::CollectionDBPath;
    qDebug() << "Getting settings info from: " << Bae::SettingPath;
    qDebug() << "Getting artwork files from: " << Bae::CachePath;


    if(!Bae::fileExists(notifyDir+"/Babe.notifyrc"))
    {
        qDebug()<<"The Knotify file does not exists, going to create it";
        QFile knotify(":Data/data/Babe.notifyrc");

        if(knotify.copy(notifyDir+"/Babe.notifyrc"))
            qDebug()<<"the knotify file got copied";
    }

    QDir collectionDBPath_dir(Bae::CollectionDBPath);
    QDir settingsPath_dir(Bae::SettingPath);
    QDir cachePath_dir(Bae::CachePath);
    QDir youtubeCache_dir(Bae::YoutubeCachePath);

    if (!collectionDBPath_dir.exists())
        collectionDBPath_dir.mkpath(".");
    if (!settingsPath_dir.exists())
        settingsPath_dir.mkpath(".");
    if (!cachePath_dir.exists())
        cachePath_dir.mkpath(".");
    if (!youtubeCache_dir.exists())
        youtubeCache_dir.mkpath(".");

    connect(&this->brainDeamon, &Deamon::Brain::artworkReady, [this] (const DB &album)
    {
        emit this->albumArtReady(album);
    });

    connect(&this->brainDeamon, &Deamon::Brain::finished, [this]()
    {
        this->movie->stop();
        ui->label->hide();
        this->ui->sourcesFrame->setEnabled(true);

    });

    connect(&this->brainDeamon, &Deamon::Brain::done, [this](const TABLE type)
    {
        emit this->refreshTables({{type,false}});
    });

    connect(this->ui->pulpoBrainz_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int &value)
    {
        qDebug()<<"interval changed to:"<<value;
        Bae::saveSettings("BRAINZ_INTERVAL",value,"SETTINGS");
    });

    connect(this->ui->pulpoBrainz_checkBox,static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), [this](const bool &value)
    {
        Bae::saveSettings("BRAINZ",value,"SETTINGS");
    });

    connect(&collection_db, &CollectionDB::trackInserted, [this]()
    {
        fileSaver.nextTrack();
        this->ui->progressBar->setValue(this->ui->progressBar->value()+1);
    });

    connect(&fileSaver,&FileLoader::collectionSize,[this](int size)
    {
        if(size>0)
        {
            ui->progressBar->setValue(1);
            ui->progressBar->setMaximum(size);
            ui->progressBar->show();

        }else
        {
            this->dirs.clear();
            this->collectionWatcher();
            this->watcher->removePaths(watcher->directories());
        }
    });

    connect(&fileSaver,&FileLoader::finished,[this]()
    {
        ui->progressBar->hide();
        ui->progressBar->setValue(0);

        collectionWatcher();
        emit refreshTables({{TABLE::TRACKS, true},{TABLE::ALBUMS, false},{TABLE::ARTISTS, false},{TABLE::PLAYLISTS, true}});
        this->fetchArt();
    });

    connect(&fileSaver, &FileLoader::trackReady, &collection_db, &CollectionDB::addTrack);

    connect(this, &settings::collectionPathChanged, this, &settings::populateDB);

    this->ytFetch = new YouTube(this);
    connect(ytFetch,&YouTube::youtubeTrackReady, this, &settings::populateDB);

    this->babeSocket = new Socket(static_cast<quint16>(Bae::BabePort.toInt()),this);
    connect(babeSocket, &Socket::message, ytFetch, &YouTube::fetch);

    ui->remove->setEnabled(false);
    ui->progressBar->hide();
    this->about_ui = new About(this);

    movie = new QMovie(this);
    movie->setFileName(":Data/data/ajax-loader.gif");
    ui->label->setMovie(movie);
    ui->label->hide();

    watcher = new QFileSystemWatcher(this);
    // watcher->addPath(youtubeCachePath);

    connect(watcher, SIGNAL(directoryChanged(QString)), this,
            SLOT(handleDirectoryChanged(QString)));
}

settings::~settings()
{
    qDebug()<<"DELETING SETTINGS";
    delete ui;
    //    delete fileSaver;
    //    delete this->brainDeamon;
}


void settings::on_collectionPath_clicked(const QModelIndex &index)
{
    ui->remove->setEnabled(true);
    this->pathToRemove = index.data().toString();
}

void settings::on_remove_clicked()
{
    qDebug() << this->pathToRemove;
    if (!this->pathToRemove.isEmpty())
    {
        if(collection_db.removeSource(this->pathToRemove))
        {
            removeSettings({"collectionPath=", this->pathToRemove});
            collectionPaths.removeAll(this->pathToRemove);

            refreshCollectionPaths();
            this->dirs.clear();
            this->collectionWatcher();
            this->watcher->removePaths(watcher->directories());            ui->remove->setEnabled(false);
            emit refreshTables({{TABLE::TRACKS, true},{TABLE::ALBUMS, true},{TABLE::ARTISTS, true},{TABLE::PLAYLISTS, true}});
        }
    }
}

void settings::refreshCollectionPaths()
{
    ui->collectionPath->clear();
    ui->collectionPath->addItems(collectionPaths);
}

void settings::on_open_clicked()
{
    QString url = QFileDialog::getExistingDirectory(this,"Select folder...", QDir().homePath()+"/Music/");

    if (!collectionPaths.contains(url) && !url.isEmpty())
    {
        ui->collectionPath->addItem(QDir(url).absolutePath());
        collectionPaths << url;
        qDebug() << "Collection dir added: " << url;
        setSettings({"collectionPath=", url});
        emit collectionPathChanged(url);
    }
}

void settings::setSettings(QStringList setting) {
    std::string strNew;
    // std::string strReplace;
    strNew = setting.at(0).toStdString() + setting.at(1).toStdString();
    bool replace = false;
    /**/
    // qDebug()<<setting.at(0);
    std::ifstream settings(Bae::SettingPath.toStdString() +
                           settingsName.toStdString());
    QStringList newline;
    std::string line;
    while (std::getline(settings, line)) {
        // qDebug()<<get_setting;
        if (QString::fromStdString(line).contains(setting.at(0))) {
            if (!QString::fromStdString(line).contains("collectionPath=")) {
                replace = true;
                newline << QString::fromStdString(strNew);
            }

        } else {
            newline << QString::fromStdString(line);
        }
    }

    if (replace) {
        std::ofstream write(Bae::SettingPath.toStdString() + settingsName.toStdString());

        for (auto ln : newline) {
            write << ln.toStdString() << std::endl;
        }

    } else {
        std::ofstream write(Bae::SettingPath.toStdString() + settingsName.toStdString(),
                            std::ios::app);
        write << strNew << std::endl;
    }
}

void settings::removeSettings(QStringList setting) {
    std::string strNew;
    // std::string strReplace;
    strNew = "";
    bool replace = false;
    /**/
    // qDebug()<<setting.at(0);
    std::ifstream settings(Bae::SettingPath.toStdString() +
                           settingsName.toStdString());
    QStringList newline;
    std::string line;
    while (std::getline(settings, line)) {
        // qDebug()<<get_setting;
        if (QString::fromStdString(line).contains(setting.at(0)) &&
                QString::fromStdString(line).contains(setting.at(1))) {

            replace = true;
            newline << QString::fromStdString(strNew);

        } else {
            newline << QString::fromStdString(line);
        }
    }

    if (replace)
    {
        std::ofstream write(Bae::SettingPath.toStdString() + settingsName.toStdString());

        for (auto ln : newline)
            write << ln.toStdString() << std::endl;


    } else
    {
        std::ofstream write(Bae::SettingPath.toStdString() + settingsName.toStdString(),
                            std::ios::app);
        write << strNew << std::endl;
    }
}

void settings::addToWatcher(QStringList paths)
{
    qDebug()<<"duplicated paths in watcher removd: "<<paths.removeDuplicates();

    if(!paths.isEmpty()) watcher->addPaths(paths);
}

void settings::collectionWatcher()
{
    auto queryTxt = QString("SELECT %1 FROM %2").arg(Bae::KEYMAP[Bae::KEY::URL],Bae::TABLEMAP[Bae::TABLE::TRACKS]);
    QSqlQuery query(queryTxt);
    while (query.next())
    {
        QString location = query.value(Bae::KEYMAP[Bae::KEY::URL]).toString();
        if(!location.startsWith(Bae::YoutubeCachePath,Qt::CaseInsensitive)) //exclude the youtube cache folder
        {
            if (!this->dirs.contains(QFileInfo(location).dir().path()) && Bae::fileExists(location)) //check if parent dir isn't already in list and it exists
            {
                QString dir = QFileInfo(location).dir().path();
                this->dirs << dir;

                QDirIterator it(dir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories); // get all the subdirectories to watch
                while (it.hasNext())
                {
                    QString subDir = QFileInfo(it.next()).path();

                    if(QFileInfo(subDir).isDir() && !this->dirs.contains(subDir) && Bae::fileExists(subDir))
                        this->dirs <<subDir;
                }

            }
        }
    }

    addToWatcher(this->dirs);
}

void settings::handleDirectoryChanged(const QString &dir)
{
    qDebug()<<"directory changed:"<<dir;

    auto wait = new QTimer(this);
    wait->setSingleShot(true);
    wait->setInterval(1000);
    connect(wait, &QTimer::timeout,[=]()
    {
        emit collectionPathChanged(dir);
        wait->deleteLater();

    });
    wait->start();

}

void settings::readSettings()
{
    std::ifstream settings(Bae::SettingPath.toStdString() +
                           settingsName.toStdString());
    std::string line;
    while (std::getline(settings, line))
    {
        auto get_setting = QString::fromStdString(line);
        // qDebug()<<get_setting;
        if (get_setting.contains("collectionPath="))
        {
            collectionPaths << get_setting.replace("collectionPath=", "");
            //            qDebug() << "Setting the cPath: "
            //                     << get_setting.replace("collectionPath=", "");
            ui->collectionPath->addItem(get_setting.replace("collectionPath=", ""));

        }
    }
}


bool settings::checkCollection()
{
    if (Bae::fileExists(Bae::CollectionDBPath + collectionDBName))
    {
        qDebug() << "The CollectionDB does exists";

        collection_db.setUpCollection(Bae::CollectionDBPath + collectionDBName);
        collectionWatcher();

        if(this->ui->pulpoBrainz_checkBox->isChecked())
            this->brainDeamon.start();

        return true;
    }

    return false;
}


void settings::createCollectionDB()
{
    qDebug() << "The CollectionDB doesn't exists. Going to create the database "
                "and tables";

    collection_db.setUpCollection(Bae::CollectionDBPath + collectionDBName);
    collection_db.prepareCollectionDB();

}

void settings::populateDB(const QString &path)
{
    qDebug() << "Function Name: " << Q_FUNC_INFO
             << "new path for database action: " << path;
    fileSaver.requestPath(path);
    this->ui->sourcesFrame->setEnabled(false);
}

void settings::fetchArt()
{
    this->brainDeamon.start();
    nof.notify("Fetching art","this might take some time depending on your collection size and internet connection speed...");
    ui->label->show();
    movie->start();
}


