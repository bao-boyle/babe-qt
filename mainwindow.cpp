#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QStringList>
#include "collectionDB.h"
#include<QSqlQuery>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMimeData>
#include <QMenu>
#include <QWidgetAction>
#include <QButtonGroup>
#include<QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QTimer>
#include <scrolltext.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //setWindowFlags(Qt::WindowStaysOnTopHint);
    ui->setupUi(this);
    this->setWindowTitle(" Babe ... \xe2\x99\xa1  \xe2\x99\xa1 \xe2\x99\xa1 ");
    this->setAcceptDrops(true);
    this->setWindowIcon(QIcon(":Data/data/babe_48.svg"));
    this->setWindowIconText("Babe...");
//this->setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    /*THE VIEWS*/

    settings_widget = new settings(); //this needs to go fist

    collectionTable = new BabeTable();
    collectionTable->passCollectionConnection(&settings_widget->getCollectionDB());
    connect(collectionTable,SIGNAL(tableWidget_doubleClicked(QStringList)),this,SLOT(addToPlaylist(QStringList)));
    connect(collectionTable,SIGNAL(songRated(QStringList)),this,SLOT(addToFavorites(QStringList)));
    connect(collectionTable,SIGNAL(enteredTable()),this,SLOT(hideControls()));
    connect(collectionTable,SIGNAL(leftTable()),this,SLOT(showControls()));
    connect(collectionTable,SIGNAL(finishedPopulating()),this,SLOT(orderTables()));
    connect(collectionTable,SIGNAL( babeIt_clicked(QStringList)),this,SLOT(addToPlaylist(QStringList)));


    favoritesTable =new BabeTable();
    connect(favoritesTable,SIGNAL(tableWidget_doubleClicked(QStringList)),this,SLOT(addToPlaylist(QStringList)));
    connect(favoritesTable,SIGNAL(enteredTable()),this,SLOT(hideControls()));
    connect(favoritesTable,SIGNAL(enteredTable()),this,SLOT(hideControls()));
    connect(favoritesTable,SIGNAL(leftTable()),this,SLOT(showControls()));
    connect(favoritesTable,SIGNAL( babeIt_clicked(QStringList)),this,SLOT(addToPlaylist(QStringList)));

    resultsTable=new BabeTable();
    resultsTable->passStyle("QHeaderView::section { background-color:#e3f4d7; }");
    resultsTable->setVisibleColumn(BabeTable::STARS);
    connect(resultsTable,SIGNAL(songRated(QStringList)),this,SLOT(addToFavorites(QStringList)));
    connect(resultsTable,SIGNAL(tableWidget_doubleClicked(QStringList)),this,SLOT(addToPlaylist(QStringList)));
    connect(resultsTable,SIGNAL(enteredTable()),this,SLOT(hideControls()));
    connect(resultsTable,SIGNAL(enteredTable()),this,SLOT(hideControls()));
    connect(resultsTable,SIGNAL(leftTable()),this,SLOT(showControls()));
    connect(resultsTable,SIGNAL( babeIt_clicked(QStringList)),this,SLOT(addToPlaylist(QStringList)));

    albumsTable = new AlbumsView();
    connect(albumsTable,SIGNAL(songClicked(QStringList)),this,SLOT(addToPlaylist(QStringList)));
    connect(albumsTable,SIGNAL(songRated(QStringList)),this,SLOT(addToFavorites(QStringList)));
    connect(albumsTable,SIGNAL(songBabeIt(QStringList)),this,SLOT(addToPlaylist(QStringList)));



    playback = new QToolBar();
    utilsBar = new QToolBar();

    settings_widget->readSettings();
    setToolbarIconSize(settings_widget->getToolbarIconSize());
    connect(settings_widget, SIGNAL(toolbarIconSizeChanged(int)), this, SLOT(setToolbarIconSize(int)));
    connect(settings_widget, SIGNAL(collectionDBFinishedAdding(bool)), this, SLOT(collectionDBFinishedAdding(bool)));
    connect(settings_widget, SIGNAL(dirChanged(QString)),this, SLOT(scanNewDir(QString)));


    if(settings_widget->checkCollection())
    {
        //collectionWatcher();
        collectionTable->populateTableView("SELECT * FROM tracks");
        favoritesTable->populateTableView("SELECT * FROM tracks WHERE stars = \"4\" OR stars =  \"5\" OR babe =  \"1\"");
        albumsTable->populateTableView(settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks ORDER by album asc"));
        populateMainList();

    }
    favoritesTable->setVisibleColumn(BabeTable::STARS);
   //
    //babes_widget= new babes();
   //


    /*THE MAIN WIDGETS*/




    /*THE STREAMING / PLAYLIST*/
    connect(updater, SIGNAL(timeout()), this, SLOT(update()));
    player->setVolume(100);
    addMusicImg = new QLabel(ui->listWidget);
    addMusicImg->setPixmap(QPixmap(":Data/data/add.png").scaled(120,120,Qt::KeepAspectRatio));
    addMusicImg->setGeometry(45,40,120,120);
    connect(ui->listWidget->model() ,SIGNAL(rowsInserted(QModelIndex,int,int)),this,SLOT(on_rowInserted(QModelIndex,int,int)));
addMusicImg->hide();
    ui->listWidget->setCurrentRow(0);

    if(ui->listWidget->count() != 0)
    {

        loadTrack();
        player->pause();
        updater->start();

    }else
    {
    addMusicImg->show();
    }

    //playback->setMovable(false);

    //this->adjustSize();
   // this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    //this->setMinimumSize (200, 250);

    //checkCollection();


    //settings_widget->setContentsMargins(0,0,0,0);
    //settings_widget->setStyleSheet("background:red;");
    //collection_db.openCollection("../player/collection.db");




   // setUpViews();



    QAction *babe, *remove;
    babe = new QAction("Babe it");
    remove = new QAction("Remove from list");
    ui->listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->listWidget->addAction(babe);
    ui->listWidget->addAction(remove);




    /*SETUP MAIN TOOLBAR*/

    auto *left_spacer = new QWidget();
    left_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *right_spacer = new QWidget();
    right_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->tracks_view->setToolTip("Search...");
    ui->mainToolBar->addWidget(ui->searchField);
    ui->searchField->setChecked(true);

    ui->mainToolBar->addWidget(left_spacer);

    ui->tracks_view->setToolTip("Collection");
    ui->mainToolBar->addWidget(ui->tracks_view);
    //ui->tracks_view->setChecked(true);

    ui->albums_view->setToolTip("Albums");
    ui->mainToolBar->addWidget(ui->albums_view);

    ui->babes_view->setToolTip("Favorites");
    ui->mainToolBar->addWidget(ui->babes_view);

    ui->playlists_view->setToolTip("Playlists");
    ui->mainToolBar->addWidget(ui->playlists_view);

    ui->queue_view->setToolTip("Queue");
    ui->mainToolBar->addWidget(ui->queue_view);

    ui->info_view->setToolTip("Info");
    ui->mainToolBar->addWidget(ui->info_view);

    ui->settings_view->setToolTip("Setings");
    ui->mainToolBar->addWidget(ui->settings_view);

    ui->mainToolBar->addWidget(right_spacer);

    ui->open_btn->setToolTip("Open...");
    ui->mainToolBar->addWidget(ui->open_btn);

    this->addToolBar(Qt::BottomToolBarArea, ui->mainToolBar);
   // this->setCentralWidget(ui->listView);



    //playback->addWidget();
    //playback->addWidget(ui->horizontalSlider);

    //playback->setIconSize(QSize(16, 16));
    //this->addToolBar(Qt::BottomToolBarArea, playback);


    /*
    status = new QToolBar();
    info=new QLabel(" Babe ... \xe2\x99\xa1  \xe2\x99\xa1 \xe2\x99\xa1 ");
    //info->setStyleSheet("color:white;");

    status->addWidget(ui->shuffle_btn);
    auto *sp = new QWidget();
    sp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    status->addWidget(sp);
    status->addWidget(info);
    auto *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    status->addWidget(spacer);
    status->addWidget(ui->hide_sidebar_btn);    
    status->setIconSize(QSize(16, 16));
    status->setMovable(false);

    this->addToolBar(Qt::BottomToolBarArea,status);
    */

    /* SEARCH BAR & UTILS BAR*/

    /*auto clearSearch = new QToolButton(ui->search);
    clearSearch->setIcon(QIcon::fromTheme("clearSearch"));
    clearSearch->setAutoRaise(false);
    clearSearch->setGeometry(50, ui->search->sizeHint().height()/2,16,16);*/
    ui->search->setClearButtonEnabled(true);
    utilsBar->setMovable(false);
    utilsBar->setContentsMargins(0,0,0,0);
    utilsBar->addWidget(ui->search);
    utilsBar->addWidget(ui->resultsPLaylist);
    utilsBar->addWidget(ui->saveResults);
    ui->resultsPLaylist->setEnabled(false);
    ui->saveResults->setEnabled(false);



    ui->search->setPlaceholderText("Search...");

    //this->addToolBar(Qt::BottomToolBarArea,utilsBar);

    /*COMPOSE THE VIEWS*/

    views = new QStackedWidget;
    views->addWidget(collectionTable);
    //auto* testing = new QLabel("albums view... todo");
    views->addWidget(albumsTable);
    views->addWidget(favoritesTable);
    views->addWidget(settings_widget);
    views->addWidget(resultsTable);


    connect(ui->tracks_view, SIGNAL(clicked()), this, SLOT(collectionView()));
    connect(ui->albums_view, SIGNAL(clicked()), this, SLOT(albumsView()));
    connect(ui->babes_view, SIGNAL(clicked()), this, SLOT(favoritesView()));
    connect(ui->playlists_view, SIGNAL(clicked()), this, SLOT(playlistsView()));
    connect(ui->queue_view, SIGNAL(clicked()), this, SLOT(queueView()));
    connect(ui->info_view, SIGNAL(clicked()), this, SLOT(infoView()));
    connect(ui->settings_view, SIGNAL(clicked()), this, SLOT(settingsView()));


    /*MAIN WINDOW*/

    layout = new QGridLayout();
    layout->setContentsMargins(6,0,6,0);
    main_widget= new QWidget();
    main_widget->setLayout(layout);
    this->setCentralWidget(main_widget);



    /*album view*/
    auto *album_widget= new QWidget();
    auto *album_view = new QGridLayout();
    album_art_frame=new QFrame();
    album_art_frame->setFrameShadow(QFrame::Raised);
    album_art_frame->setFrameShape(QFrame::StyledPanel);
    //album_art_frame->setFixedSize(210,210);
    //album_art->setGeometry(0,0,100,100);

    album_art = new QLabel(album_art_frame);
    //album_art->setStyleSheet("border:1px solid #333");
    this->setCoverArt(":/Data/data/cover.jpg");
   // qDebug()<< QDir::current()<<"/cover.jpg";
    //connect(album_art,SIGNAL(clicked()),this,SLOT(labelClicked()));
    /* PLAYBACK CONTROL BOX*/





 // ui->hide_sidebar_btn->setBackgroundRole(QPalette :: Dark);
    ui->hide_sidebar_btn->setToolTip("Go Mini");
    playback->addWidget(ui->hide_sidebar_btn);

    playback->addWidget(ui->backward_btn);
    playback->addWidget(ui->fav_btn);
    playback->addWidget(ui->play_btn);
    playback->addWidget(ui->foward_btn);

    ui->shuffle_btn->setToolTip("Shuffle");
    playback->addWidget(ui->shuffle_btn);



    controls = new QWidget(album_art);
    seekBar = new QSlider(album_art);
    seekBar->setMaximum(1000);
    seekBar->setOrientation(Qt::Horizontal);
    seekBar->setGeometry(0,193,200,7);
    seekBar->setStyleSheet("QSlider\n{\nbackground:transparent;\n}\nQSlider::groove:horizontal {\nborder: 1px solid #bbb;\nbackground: white;\nheight: 5px;\nborder-radius: 4px;\n}\n\nQSlider::sub-page:horizontal {\nbackground: #f85b79;\n\nborder: 1px solid #777;\nheight: 5px;\nborder-radius: 4px;\n}\n\nQSlider::add-page:horizontal {\nbackground: #fff;\nborder: 1px solid #777;\nheight: 5px;\nborder-radius: 4px;\n}\n\nQSlider::handle:horizontal {\nbackground: #f85b79;\n\nwidth: 8px;\n\n}\n\nQSlider::handle:horizontal:hover {\nbackground: qlineargradient(x1:0, y1:0, x2:1, y2:1,\n    stop:0 #fff, stop:1 #ddd);\nborder: 1px solid #444;\nborder-radius: 4px;\n}\n\nQSlider::sub-page:horizontal:disabled {\nbackground: #bbb;\nborder-color: #999;\n}\n\nQSlider::add-page:horizontal:disabled {\nbackground: #eee;\nborder-color: #999;\n}\n\nQSlider::handle:horizontal:disabled {\nbackground: #eee;\nborder: 1px solid #aaa;\nborder-radius: 4px;\n}");
    connect(seekBar,SIGNAL(sliderMoved(int)),this,SLOT(on_seekBar_sliderMoved(int)));


    auto controls_layout = new QGridLayout();
    controls->setLayout(controls_layout);
    controls->setGeometry(100-75,75,150,50);
    controls->setStyleSheet(" QWidget{background-color: rgba(255, 255, 255, 230); border-radius:6px;} QWidget:hover{background-color:white;}");

//ui->seekBar->setStyleSheet("background:transparent; ");
    album_view->addWidget(album_art, 0,0,Qt::AlignTop);
    album_view->addWidget(ui->listWidget,1,0);
    album_view->setContentsMargins(0,0,0,0);

    controls_layout->addWidget(playback,0,0,Qt::AlignHCenter);
    //controls_layout->addWidget(ui->seekBar,1,0,Qt::AlignTop);



   album_widget->setStyleSheet("QWidget { padding:0; margin:0;  }");
    //album_art->setStyleSheet("background-color:red; padding:0; margin:0;");
   // album_art->setStyleSheet("border: 1px solid #333;");
    playback->setStyleSheet(" background:transparent; border:none;");


    //album_widget->setLayout(album_view);
    album_art_frame->setLayout(album_view);
    album_widget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding  );


    layout->addWidget(views, 0,0 );
    layout->addWidget(utilsBar, 1,0 );
    layout->addWidget(album_art_frame,0,1,0,1, Qt::AlignRight);
    //this->setStyle();
    go_mini();
}





MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::labelClicked()
{
    qDebug()<<"the label got clicked";
}


 void MainWindow::enterEvent(QEvent *event)
{
    //qDebug()<<"entered the window";
   showControls();
   if (mini_mode==2) this->setWindowState(Qt::WindowActive);

}


 void MainWindow::leaveEvent(QEvent *event)
{
    //qDebug()<<"left the window";
    hideControls();

    //timer = new QTimer(this);
      /*connect(timer, SIGNAL(timeout()), this, SLOT(hideControls()));

      connect(timer,SIGNAL(timeout()), this, [&timer, this]() {
          qDebug()<<"ime is up";
          timer->stop();
      });*/

        //timer->start(3000);


}

 void MainWindow::hideControls()
 {
     //qDebug()<<"ime is up";
     controls->hide();
    // timer->stop();*/
 }

 void MainWindow::showControls()
 {
     //qDebug()<<"ime is up";
     controls->show();
    // timer->stop();*/
 }
  void	MainWindow::dragEnterEvent(QDragEnterEvent *event)
 {
     event->accept();
 }

  void	MainWindow::dragLeaveEvent(QDragLeaveEvent *event){
     event->accept();
 }

  void	MainWindow::dragMoveEvent(QDragMoveEvent *event)
 {
     event->accept();
 }

  void	MainWindow::dropEvent(QDropEvent *event)
 {
     QList<QUrl> urls;
     urls = event->mimeData()->urls();
    QStringList list;
     for( auto url  : urls)
     {
         //qDebug()<<url.path();

         if(QFileInfo(url.path()).isDir())
         {
             //QDir dir = new QDir(url.path());
                 QDirIterator it(url.path(), QStringList() << "*.mp4" << "*.mp3" << "*.wav" <<"*.flac" <<"*.ogg" << "*.m4a", QDir::Files, QDirIterator::Subdirectories);
                 while (it.hasNext())
                 {
                     list<<it.next();

                     //qDebug() << it.next();
                 }

         }else if(QFileInfo(url.path()).isFile())
         {
         list<<url.path();
         }


     }

     playlist.add(list);
     updateList();
     //populateTableView();
     //ui->save->setChecked(false);
     if(shuffle) shufflePlaylist();
 }
void MainWindow::setCoverArt(QString path)
{
    album_art->setPixmap(QPixmap(path).scaled(200,200,Qt::KeepAspectRatio));

}

void MainWindow::addToFavorites(QStringList list)
{
    favoritesTable->addRow(list.at(0),list.at(1),list.at(2),list.at(3),list.at(4),list.at(5));
    qDebug()<<list.at(0)<<list.at(1)<<list.at(2)<<list.at(3)<<list.at(4)<<list.at(5);
}

void MainWindow::addToCollection(QStringList list)
{
    collectionTable->addRow(list.at(0),list.at(1),list.at(2),list.at(3),list.at(4),list.at(5));
    qDebug()<<list.at(0)<<list.at(1)<<list.at(2)<<list.at(3)<<list.at(4)<<list.at(5);
}

void MainWindow::setToolbarIconSize(int iconSize)
{
    qDebug()<< "Toolbar icons size changed";
    ui->mainToolBar->setIconSize(QSize(iconSize,iconSize));
    playback->setIconSize(QSize(iconSize,iconSize));
    utilsBar->setIconSize(QSize(iconSize,iconSize));
    ui->mainToolBar->update();
    playback->update();
   // this->update();
}

void MainWindow::setUpViews()
{

}


void MainWindow::collectionView()
{
    qDebug()<< "All songs view";
    views->setCurrentIndex(0);
    if(mini_mode!=0) expand();

}

void MainWindow::albumsView()
{
    views->setCurrentIndex(1);
    //if(hideSearch)utilsBar->show();
    if(mini_mode!=0) expand();
}
void MainWindow::playlistsView()
{
    views->setCurrentIndex(3);
    if(mini_mode!=0) expand();
}
void MainWindow::queueView()
{
    views->setCurrentIndex(1);
    if(mini_mode!=0) expand();
}
void MainWindow::infoView()
{

    views->setCurrentIndex(0);

    if(mini_mode!=0) expand();
    //if(!hideSearch)utilsBar->hide();
}
void MainWindow::favoritesView()
{
    views->setCurrentIndex(2);
    if(mini_mode!=0) expand();

   /* QString url= QFileDialog::getExistingDirectory();

qDebug()<<url;

    QStringList urlCollection;
//QDir dir = new QDir(url);
    QDirIterator it(url, QStringList() << "*.mp4" << "*.mp3" << "*.wav" <<"*.flac" <<"*.ogg", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        urlCollection<<it.next();

        //qDebug() << it.next();
    }

   // collection.add(urlCollection);
    //updateList();
    populateTableView();*/
}
void MainWindow::settingsView()
{
    views->setCurrentIndex(3);
    if(mini_mode!=0) expand();
    //if(!hideSearch) utilsBar->hide();
}

void MainWindow::expand()
{
    views->show();
    utilsBar->show();
    ui->searchField->setChecked(true);
    hideSearch=false;
    album_art_frame->setFrameShadow(QFrame::Raised);
    album_art_frame->setFrameShape(QFrame::StyledPanel);
    layout->setContentsMargins(6,0,6,0);
    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    this->setMinimumSize(750,450);
    //qDebug()<<this->minimumWidth()<<this->minimumHeight();

    this->adjustSize();
    ui->hide_sidebar_btn->setToolTip("Go Mini");

    ui->hide_sidebar_btn->setIcon(QIcon(":Data/data/full_mode.svg"));
    ui->mainToolBar->actions().at(0)->setVisible(true);
    ui->mainToolBar->actions().at(8)->setVisible(true);

     mini_mode=0;
//keepOnTop(false);
}

void MainWindow::go_mini()
{
    //this->setMaximumSize (0, 0);

    views->hide();
    utilsBar->hide();
    ui->searchField->setChecked(false);
    hideSearch=true;
    ui->mainToolBar->actions().at(0)->setVisible(false);
    ui->mainToolBar->actions().at(8)->setVisible(false);
    album_art_frame->setFrameShadow(QFrame::Plain);
    album_art_frame->setFrameShape(QFrame::NoFrame);
    layout->setContentsMargins(0,0,0,0);
   // ui->searchField->setVisible(false);
    this->resize(minimumSizeHint());
    main_widget->resize(minimumSizeHint());
    this->setFixedSize(minimumSizeHint());
    ui->hide_sidebar_btn->setToolTip("Go Extra-Mini");
    ui->hide_sidebar_btn->setIcon(QIcon(":Data/data/mini_mode.svg"));
    mini_mode=1;
//keepOnTop(true);

}

void MainWindow::keepOnTop(bool state)
{

    if (state)
        {//Qt::WindowFlags flags = windowFlags();
setWindowFlags(Qt::WindowStaysOnTopHint);
show();
    }else
    {
        //setWindowFlags(~ Qt::WindowStaysOnTopHint);
       //show();
    }
}

void MainWindow::setStyle()
{

   /* ui->mainToolBar->setStyleSheet(" QToolBar { border-right: 1px solid #575757; } QToolButton:hover { background-color: #d8dfe0; border-right: 1px solid #575757;}");
    playback->setStyleSheet("QToolBar { border:none;} QToolBar QToolButton { border:none;} QToolBar QSlider { border:none;}");
    this->setStyleSheet("QToolButton { border: none; padding: 5px; }  QMainWindow { border-top: 1px solid #575757; }");*/
    //status->setStyleSheet("QToolButton { color:#fff; } QToolBar {background-color:#575757; color:#fff; border:1px solid #575757;} QToolBar QLabel { color:#fff;}" );

}



void MainWindow::on_hide_sidebar_btn_clicked()
{
    if(mini_mode==0)
    {
        go_mini();

    }else if(mini_mode==1)
    {

        ui->listWidget->hide();
        ui->mainToolBar->hide();
       // ui->mainToolBar->hide();
       // ui->tableWidget->hide();
        //this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        main_widget->resize(minimumSizeHint());
        this->resize(minimumSizeHint());

        this->setFixedSize(200,200);

        album_art->setStyleSheet("QLabel{border: 1px solid #777;}");
        layout->setContentsMargins(0,0,0,0);

        this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        this->show();
        ui->hide_sidebar_btn->setToolTip("Expand");

        mini_mode=2;

    }else if(mini_mode==2)
    {
         ui->mainToolBar->show();
        ui->listWidget->show();
        this->resize(minimumSizeHint());
        main_widget->resize(minimumSizeHint());
        this->setFixedSize(minimumSizeHint());
      //  this->adjustSize();
        ui->hide_sidebar_btn->setToolTip("Full View");
        //layout->setContentsMargins(6,0,6,0);

        album_art->setStyleSheet("QLabel{border: none}");
        ui->hide_sidebar_btn->setIcon(QIcon(":Data/data/full_mode.svg"));


        this->setWindowFlags(Qt::Widget | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
        this->show();
        mini_mode=3;
    }else if (mini_mode==3)
    {
        expand();
    }


}

void MainWindow::on_shuffle_btn_clicked()
{
    /*state 0: media-playlist-consecutive-symbolic
            1: media-playlist-shuffle
            2:media-playlist-repeat-symbolic
    */
    if(shuffle_state==0)
    {
        shuffle = true;
        shufflePlaylist();
        ui->shuffle_btn->setIcon(QIcon(":Data/data/media-playlist-shuffle.svg"));
        ui->shuffle_btn->setToolTip("Repeat");
        shuffle_state=1;

    }else if (shuffle_state==1)
    {

        repeat = true;
        ui->shuffle_btn->setIcon(QIcon(":Data/data/media-playlist-repeat.svg"));
        ui->shuffle_btn->setToolTip("Consecutive");
        shuffle_state=2;


    }else if(shuffle_state==2)
    {
        repeat = false;
        shuffle = false;
        ui->shuffle_btn->setIcon(QIcon(":Data/data/view-media-playlist.svg"));
        ui->shuffle_btn->setToolTip("Shuffle");
        shuffle_state=0;
    }



}

void MainWindow::on_open_btn_clicked()
{
    //bool startUpdater = false;

    //if(ui->listWidget->count() == 0) startUpdater = true;




      QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Music Files"),QDir().homePath()+"/Music/", tr("Audio (*.mp3 *.wav *.mp4 *.flac *.ogg *.m4a)"));
    if(!files.empty())
    {



        playlist.add(files);
        updateList();
        //populateTableView();
        //ui->save->setChecked(false);
        if(shuffle) shufflePlaylist();
        //if(startUpdater) updater->start();
    }
}



void MainWindow::populateMainList()
{
    QSqlQuery query= settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks WHERE babe = 1");

    QStringList files;
       while (query.next())
       {



        files << query.value(3).toString();

       }

       playlist.add(files);
       updateList();
       //populateTableView();
       //ui->save->setChecked(false);
       if(shuffle) shufflePlaylist();
}

void MainWindow::updateList()
{
    ui->listWidget->clear();
     ui->listWidget->addItems(playlist.getTracksNameList());
    /*for(auto str : playlist.getTracksNameList())
    {
        auto label =new ScrollText();
        label->setText(str);
        //label->setFixedHeight(40);
        label->setMaxSize(200);
       // label->setStyleSheet("color:red;");
        auto item =new QListWidgetItem();

        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item,label);
    }*/


}

void MainWindow::on_listWidget_doubleClicked(const QModelIndex &index)
{
    lCounter = getIndex();

    //ui->play_btn->setChecked(false);
    //ui->searchBar->clear();
    loadTrack();
    player->play();
    updater->start();
    playing= true;
    ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));

}



void MainWindow::loadTrack()
{
     current_song_url = QString::fromStdString(playlist.tracks[getIndex()].getLocation());
     player->setMedia(QUrl::fromLocalFile(current_song_url));
     auto qstr = QString::fromStdString(playlist.tracks[getIndex()].getTitle()+" \xe2\x99\xa1 "+playlist.tracks[getIndex()].getArtist());
     this->setWindowTitle(qstr);

     //here check if the song to play is already babe'd and if so change the icon
      if(settings_widget->getCollectionDB().checkQuery("SELECT * FROM tracks WHERE location = \""+current_song_url+"\" AND babe = \"1\""))
      {
          ui->fav_btn->setIcon(QIcon(":Data/data/loved.svg"));
      }else
      {
          ui->fav_btn->setIcon(QIcon(":Data/data/love-amarok"));
      }




     qDebug()<<"Current song playing is: "<< current_song_url;
}

int MainWindow::getIndex()
{
    return ui->listWidget->currentIndex().row();
}



void MainWindow::on_seekBar_sliderMoved(int position)
{
    player->setPosition(player->duration() / 1000 * position);
}






void MainWindow::update()
{   if(!seekBar->isSliderDown())
        seekBar->setValue((double)player->position()/player->duration() * 1000);

    if(player->state() == QMediaPlayer::StoppedState)
    {
        next();
    }
}

void MainWindow::next()
{
    lCounter++;

    if(repeat)
    {
        lCounter--;
    }

    if(lCounter >= ui->listWidget->count())
        lCounter = 0;

    (!shuffle or repeat) ? ui->listWidget->setCurrentRow(lCounter) : ui->listWidget->setCurrentRow(shuffledPlaylist[lCounter]);

    //ui->play->setChecked(false);
    //ui->searchBar->clear();

    loadTrack();
    player->play();

}

void MainWindow::back()
{
     lCounter--;

     if(lCounter < 0)
        lCounter = ui->listWidget->count() - 1;


     (!shuffle) ? ui->listWidget->setCurrentRow(lCounter) : ui->listWidget->setCurrentRow(shuffledPlaylist[lCounter]);

     //ui->play->setChecked(false);
     //ui->searchBar->clear();

     loadTrack();
     player->play();
}

void MainWindow::shufflePlaylist()
{
    shuffledPlaylist.resize(0);

    for(int i = 0; i < ui->listWidget->count(); i++)
    {
        shuffledPlaylist.push_back(i);
    }

    random_shuffle(shuffledPlaylist.begin(), shuffledPlaylist.end());
}

void MainWindow::on_play_btn_clicked()
{
    if(ui->listWidget->count() != 0)
    {
        if(player->state() == QMediaPlayer::PlayingState)
        {
            player->pause();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-start.svg"));
        }
       else
       {
            player->play();
            updater->start();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
       }
      }
}

void MainWindow::on_backward_btn_clicked()
{
    if(ui->listWidget->count() != 0)
    {
        if(player->position() > 3000)
        {
           player->setPosition(0);
        }
        else
        {

            back();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
        }
     }
}

void MainWindow::on_foward_btn_clicked()
{
    if(ui->listWidget->count() != 0)
    {
        if(repeat)
        {
            repeat = !repeat;next();repeat = !repeat;
        }
        else
        {
            next();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
        }
     }
}


void MainWindow::collectionDBFinishedAdding(bool state)
{
    if(state)
    {
        qDebug()<<"now it i time to put the tracks in the table ;)";
        //settings_widget->getCollectionDB().closeConnection();
        collectionTable->flushTable();
        collectionTable->populateTableView( "SELECT * FROM tracks");
        albumsTable->flushGrid();
        albumsTable->populateTableView(settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks ORDER by album asc"));


    }
}

void MainWindow::orderTables()
{
    favoritesTable->setTableOrder(4,BabeTable::DESCENDING);
    collectionTable->setTableOrder(1,BabeTable::ASCENDING);
    qDebug()<<"finished populating tables, now ordering them";
}





void MainWindow::on_fav_btn_clicked()
{


    if(settings_widget->getCollectionDB().checkQuery("SELECT * FROM tracks WHERE location = \""+current_song_url+"\" AND babe = \"1\""))
    {
        //ui->fav_btn->setIcon(QIcon::fromTheme("face-in-love"));
        qDebug()<<"The song is already babed";
        if(settings_widget->getCollectionDB().insertInto("tracks","babe",current_song_url,0))
        {
            ui->fav_btn->setIcon(QIcon(":Data/data/love-amarok.svg"));

        }

    }else
    {

                  if(settings_widget->getCollectionDB().check_existance("tracks","location",current_song_url))
                  {
                      if(settings_widget->getCollectionDB().insertInto("tracks","babe",current_song_url,1))
                      {
                          ui->fav_btn->setIcon(QIcon(":Data/data/loved.svg"));

                      }
                      qDebug()<<"trying to babe sth";
                  }else
                  {
                     qDebug()<<"Sorry but that song is not in the database";

                     addToCollectionDB({current_song_url},"1");


                       ui->fav_btn->setIcon(QIcon(":Data/data/loved.svg"));

                       //ui->tableWidget->insertRow(ui->tableWidget->rowCount());



                      //to-do: create a list and a tracks object and send it the new song and then write that track list into the database
                  }
                  addToFavorites({QString::fromStdString(playlist.tracks[getIndex()].getTitle()),QString::fromStdString(playlist.tracks[getIndex()].getArtist()),QString::fromStdString(playlist.tracks[getIndex()].getAlbum()),QString::fromStdString(playlist.tracks[getIndex()].getLocation()),"\xe2\x99\xa1","1"});

    }






}
  void MainWindow::scanNewDir(QString url)
{
     QStringList list;
      QDirIterator it(url, QStringList() << "*.mp4" << "*.mp3" << "*.wav" <<"*.flac" <<"*.ogg" <<"*.m4a", QDir::Files, QDirIterator::Subdirectories);
      while (it.hasNext())
      {
        QString song = it.next();
          if(!settings_widget->getCollectionDB().check_existance("tracks","location",song))
          {

         // qDebug()<<"New music files recently added: "<<it.next();
          list<<song;
          }

          //qDebug() << it.next();
      }


      addToCollectionDB_t(list);

 }
void MainWindow::addToCollectionDB(QStringList url, QString babe)
{
    Playlist song;
      song.add(url);
      for(auto ui: song.getTracks()) qDebug()<< QString::fromStdString(ui.getTitle());

 settings_widget->getCollectionDB().addSong(song.getTracks(),babe.toInt());
 // addToCollection({QString::fromStdString(song.tracks[getIndex()].getTitle()),QString::fromStdString(song.tracks[getIndex()].getArtist()),QString::fromStdString(song.tracks[getIndex()].getAlbum()),QString::fromStdString(song.tracks[getIndex()].getLocation()),"\xe2\x99\xa1",babe});
 collectionTable->flushTable();
 collectionTable->populateTableView("SELECT * FROM tracks");
 albumsTable->flushGrid();
 albumsTable->populateTableView(settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks ORDER by album asc"));
}
//iterates through the paths of the modify folders tp search for new music and then refreshes the collection view
void MainWindow::addToCollectionDB_t(QStringList url)
{
    Playlist song;
      song.add(url);
     // for(auto ui: song.getTracks()) qDebug()<< QString::fromStdString(ui.getLocation());
settings_widget->getCollectionDB().addSong(song.getTracks(),0);
 //settings_widget->getCollectionDB().addTrack();

/*for(auto track :song.getTracks() )
{
addToCollection({QString::fromStdString(track.getTitle()),QString::fromStdString(track.getArtist()),QString::fromStdString(track.getAlbum()),QString::fromStdString(track.getLocation()),"\xe2\x99\xa1",0});
}*/
collectionTable->flushTable();
collectionTable->populateTableView("SELECT * FROM tracks");
albumsTable->flushGrid();
albumsTable->populateTableView(settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks ORDER by album asc"));

}

void MainWindow::on_searchField_clicked()
{

    if(hideSearch)
    {
        utilsBar->hide();
        ui->searchField->setChecked(false);
        hideSearch=false;

    }else
    {

        utilsBar->show();
        ui->searchField->setChecked(true);
        hideSearch=true;

    }

    if(mini_mode!=0)
    {
        expand();
        utilsBar->show();
        ui->searchField->setChecked(true);
        hideSearch=true;
    }
}


void MainWindow::addToPlaylist(QStringList list)
{
    qDebug()<<"hayatuususu";

    playlist.add(list);
        updateList();

        if(shuffle) shufflePlaylist();
}

void MainWindow::on_search_returnPressed()
{

    if(ui->search->text().size()!=0) views->setCurrentIndex(4);


}

void MainWindow::on_search_textChanged(const QString &arg1)
{
    QString search=arg1;
    if(search.size()!=0)
    {
        views->setCurrentIndex(4);
        qDebug()<<search;
        resultsTable->flushTable();
        resultsTable->populateTableView("SELECT * FROM tracks WHERE title LIKE '%"+search+"%' OR artist LIKE '%"+search+"%' OR album LIKE '%"+search+"%'");
        ui->resultsPLaylist->setEnabled(true);
        ui->saveResults->setEnabled(true);

    }else
    {
        views->setCurrentIndex(0);
        ui->tracks_view->setChecked(true);
        ui->resultsPLaylist->setEnabled(false);
        ui->saveResults->setEnabled(false);
    }

}



void MainWindow::on_resultsPLaylist_clicked()
{

    addToPlaylist(resultsTable->getTableContent(3));
}

void MainWindow::on_settings_view_clicked()
{
    setCoverArt(":Data/data/babe.png");
}



void MainWindow::on_rowInserted(QModelIndex model ,int x,int y)
{
    qDebug()<<"indexes moved";
    addMusicImg->hide();
}
