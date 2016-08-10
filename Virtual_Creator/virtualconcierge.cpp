/* Copyright 2015 Ruan Luies */

#include <QDebug>
#include <QFileDialog>
#include <QStringList>
#include <QSurfaceFormat>
#include "./Network/client.h"
#include "./virtualconcierge.h"
#include "./ui_virtualconcierge.h"
#include "./SMTP/smtp.h"

VirtualConcierge::VirtualConcierge(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VirtualConcierge),
    show_email(false),
    show_access(false),
    enable_feet(false),
    enable_vehicle(false),
    enable_wheelchair(false),
    enable_bicycle(false),
    display_feet(false),
    display_vehicle(false),
    display_wheelchair(false),
    display_bicycle(false),
    display_pause_play(false),
    max_waiting(0),
    reset_frequency(0),
    reset_counter(300){
    ui->setupUi(this);
    ui->button_play_pause->hide();
    QPalette* palette = new QPalette();
    if (PremisesExporter::fileExists("VirtualConcierge/background.png"))
      palette->setBrush(QPalette::Background,*(new QBrush(*(new QPixmap("VirtualConcierge/background.png")))));
    else
      palette->setBrush(QPalette::Background,*(new QBrush(*(new QPixmap(":/BackGround_Virtual_Concierge")))));
    setPalette(*palette);
    load_interface("VirtualConcierge/nodes.pvc",
                   "VirtualConcierge/directories.dir");
    reset_timer = new QTimer(this);
    sync_with_server = new QTimer(this);
    connect(this, SIGNAL(find_path(int, int)),
            ui->openGLWidget, SLOT(find_path(int, int)));
    connect(this, SIGNAL(send_access(bool, bool, bool, bool)),
            ui->openGLWidget, SLOT(receive_access(bool, bool, bool, bool)));
    connect(this, SIGNAL(disable_antialiasing(bool)),
            ui->openGLWidget, SLOT(antialiasing(bool)));
    connect(this->reset_timer, SIGNAL(timeout()),
            this, SLOT(reset_timer_count()));
    connect(this, SIGNAL(reset_everything()),
            this->ui->openGLWidget, SLOT(reset_everything()));
    connect(this, SIGNAL(pause(bool)),
            ui->openGLWidget, SLOT(pause(bool)));
    connect(this, SIGNAL(change_movement_speed(float)),
            ui->openGLWidget, SLOT(change_speed(float)));
    connect(this, SIGNAL(reload_everything()),
            ui->openGLWidget, SLOT(reload_everything()));
    connect(this->sync_with_server, SIGNAL(timeout()),
            ui->openGLWidget, SLOT(reload_everything()));
    connect(this->sync_with_server, SIGNAL(timeout()),
            this, SLOT(reload_interface()));
    create_interface();
    load_config("config.config");
    //reset_timer->start(1000);
    //if ( reset_frequency != 0)
    //  sync_with_server->start(reset_frequency);

}

void VirtualConcierge::mousePressEvent ( QMouseEvent * ) {
  // change the index of the stacked widget
  if (ui->stackedWidget->currentIndex() == 1 ) {
    this->ui->stackedWidget->setCurrentIndex(0);
  }
}

bool VirtualConcierge::copyRecursively(const QString &srcFilePath,
                            const QString &tgtFilePath)
{
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        if (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName()))
            return false;
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames) {
            const QString newSrcFilePath
                    = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath
                    = tgtFilePath + QLatin1Char('/') + fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        if (!QFile::copy(srcFilePath, tgtFilePath))
            return false;
    }
    return true;
}

void VirtualConcierge::reset_timer_count() {
  max_waiting++;
  if ( max_waiting > reset_counter ) {
    ui->button_play_pause->setChecked(false);
    ui->button_play_pause->setText("Playing");
    emit pause(false);
    emit reset_everything();
      // resizes temp array, but does not release memory!!
     this->temp.resize(0);

      // hide the buttons from the first page
      foreach(NodeButton* button, this->catagory_)
          button->hide();
      foreach(NodeButton* button, this->buttons_)
          button->hide();
      foreach(NodeButton* button, this->directories_)
          button->hide();

      load_interface("VirtualConcierge/nodes.pvc",
                     "VirtualConcierge/directories.dir");
      create_interface();
      this->ui->stackedWidget->setCurrentIndex(1);
      max_waiting = 0;
  }
}

void VirtualConcierge::reload_interface(){
    ui->button_play_pause->setChecked(false);
    ui->button_play_pause->setText("Playing");
    emit pause(false);
    emit reset_everything();
      // resizes temp array, but does not release memory!!
     this->temp.resize(0);

      // hide the buttons from the first page
      foreach(NodeButton* button, this->catagory_)
          button->hide();
      foreach(NodeButton* button, this->buttons_)
          button->hide();
      foreach(NodeButton* button, this->directories_)
          button->hide();

      load_interface("VirtualConcierge/nodes.pvc",
                     "VirtualConcierge/directories.dir");
      create_interface();
}

void VirtualConcierge::logged_in(QByteArray session, bool value) {
  this->session_ = session;
  if( value ) {
    while(!clearDir("VirtualConcierge"));
  }
}

bool VirtualConcierge::clearDir( const QString path ) {
    QDir dir( path );
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
    foreach( QString dirItem, dir.entryList() )
    {
        if( dir.remove( dirItem ) )
        {
            qDebug() << "Deleted - " + path + QDir::separator() + dirItem ;
        }
        else
        {
            qDebug() << "Fail to delete - " + path+ QDir::separator() + dirItem;
        }
    }
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    foreach( QString dirItem, dir.entryList() ) {
        QDir subDir( dir.absoluteFilePath( dirItem ) );
        if( subDir.removeRecursively() ) {
            qDebug() << "Deleted - All files under " + dirItem ;
        }
        else {
            qDebug() << "Fail to delete - Files under " + dirItem;
        }
    }
    return true;
}

void VirtualConcierge::load_config(QString file_name) {
    if ( PremisesExporter::fileExists("VirtualConcierge/" + file_name) ) {

      // load the text file
      QFile textfile("VirtualConcierge/" + file_name);
      // open the text file
      textfile.open(QIODevice::ReadOnly | QIODevice::Text);
      QTextStream ascread(&textfile);

      if ( textfile.isOpen() ) {
        // read each line of the file
        QString line = ascread.readLine();
        while ( !line.isNull() ) {
          // break the line up in usable parts
          QStringList list = line.split("=");

          // check the type of line
          if ( list[0] == "email" ) {
            QString result = "no";

            // add email access
            if ( list.count() > 1 )
            result = list[1];
            if ( result == "yes") {
              show_email = true;
            }
            emit disable_antialiasing(show_email);
          } else if ( list[0] == "accessibility" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                show_access = true;
              }
          } else if ( list[0] == "name" ) {
            QString result = "";

            // add email access
            if ( list.count() > 1 )
            result = list[1];
            ui->label_name->setText(result);
          } else if ( list[0] == "enable_feet" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                enable_feet = true;
              }
          } else if ( list[0] == "enable_vehicle" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                enable_vehicle = true;
              }
          } else if ( list[0] == "enable_wheelchair" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                enable_wheelchair = true;
              }
          } else if ( list[0] == "enable_bicycle" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                enable_bicycle = true;
              }
          } else if ( list[0] == "display_bicycle" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                display_bicycle = true;
              }
          } else if ( list[0] == "display_vehicle" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                display_vehicle = true;
              }
          } else if ( list[0] == "display_wheelchair" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                display_wheelchair = true;
              }
          } else if ( list[0] == "display_feet" ) {
              QString result = "no";

              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                display_feet = true;
              }
          } else if (list[0] == "reset_timer") {
              if ( list.count() > 1 ) {
                  if ( list[1].toInt() > 0 ) {
                    reset_counter = list[1].toInt();
                    reset_timer->start(1000);
                  }
                  else
                      reset_counter = 999999;
              }
          } else if (list[0] == "pause_play") {
              QString result = "no";
              // add email access
              if ( list.count() > 1 )
              result = list[1];
              if ( result == "yes") {
                display_pause_play = true;
              }
          } else if (list[0] == "movement_speed") {
              if ( list.count() > 1 ) {
                  if ( list[1].toInt() > 0 )
                    change_movement_speed(list[1].toFloat());
                  else
                    change_movement_speed(1.0f);
              }
          } else if ( list[0] == "font_color") {
              if ( list.count() > 1 ) {
                 qApp->setStyleSheet("QLabel { color:"+ list[1]+"; };\n");
               }
          } else if ( list[0] == "button_font_color") {
              if ( list.count() > 1 ) {
                ui->stackedWidget->widget(0)->setStyleSheet( "QPushButton{ color:" + list[1] + "; };");
              }
          } else if ( list[0] == "login") {
              if ( list.count() > 1 ) {
                QStringList ls = list[1].split(",");
                if ( ls.count() > 1 ) {
                  Client *client_logging = new Client();
                  client_logging->Login(ls[0],ls[1]);
                  connect(client_logging, SIGNAL(logged_in(QByteArray, bool)),
                          this, SLOT(logged_in(QByteArray, bool)));
                  emit reload_everything();
                }
              }
          } else if (list[0] == "reset_frequency") {
              if ( list.count() > 1 ) {
                  if ( list[1].toInt() > 0 ) {
                      reset_frequency=list[1].toInt();
                  if ( reset_frequency != 0)
                    sync_with_server->start(reset_frequency);
                  } else {
                    sync_with_server->stop();
                  }
              }
          }

        // read next line
        line = ascread.readLine();
        }

        // close the textfile
        textfile.close();
      }
    }
    if ( !show_email ) {
        ui->lineEdit_email->hide();
        ui->pushButton_send_mail->hide();
    } else {
        ui->lineEdit_email->show();
        ui->pushButton_send_mail->show();
    }
    // hide all the buttons initially
    ui->button_bicycle->hide();
    ui->button_feet->hide();
    ui->button_other_vehicle->hide();
    ui->button_wheelchair->hide();

    if ( show_access ) {
        // enable the buttons from the config file
        ui->button_bicycle->setChecked(this->enable_bicycle);
        ui->button_other_vehicle->setChecked(this->enable_vehicle);
        ui->button_feet->setChecked(this->enable_feet);
        ui->button_wheelchair->setChecked(this->enable_wheelchair);

        if ( this->display_bicycle )
         ui->button_bicycle->show();
        if ( this->display_feet )
          ui->button_feet->show();
        if ( this->display_vehicle )
          ui->button_other_vehicle->show();
        if ( this->display_wheelchair )
          ui->button_wheelchair->show();
    }
    if ( this->display_pause_play )
      ui->button_play_pause->show();
    // set the access paths from the config file
    emit send_access(this->enable_wheelchair,
                     this->enable_feet,
                     this->enable_bicycle,
                     this->enable_vehicle);
}

void VirtualConcierge::get_button_value(int value, bool findvalue) {
    this->max_waiting = 0;
    // 0 is the starting position
    if ( !findvalue )
        emit find_path(0, value);
    else
        show_new_interface(value);
}

void VirtualConcierge::show_new_interface(int value) {
      // resizes temp array, but does not release memory!!
     this->temp.resize(0);

      // hide the buttons from the first page
      foreach(NodeButton* button, this->catagory_)
          button->hide();
      foreach(NodeButton* button, this->buttons_)
          button->hide();
      foreach(NodeButton* button, this->directories_)
          button->hide();

      // add child path locations
      if ( value < this->node_list_.count() ) {
          QStringList node_ls = this->node_list_.value(value).split(";");
          const int count = node_ls.count();
          for ( int y = 0; y < count; y++ ) {
             int node_index = (node_ls[y] != "") ? node_ls[y].toInt() : (-1);
              if ( node_index > -1 ) {
                  int index_from_list =
                          get_index_from_index(this->buttons_, node_index);
                  if ( index_from_list > -1 ) {
                      NodeButton* button =
                              this->buttons_.value(index_from_list);
                      button->show();
                     this->temp.push_back(button);
                    }
              }
          }
      }

      // add child directories
      if ( value < this->directory_list_.count() ) {
          QStringList dir_ls = this->directory_list_.value(value).split(";");
          const int count = dir_ls.count();
          for ( int x = 0; x < count; x++ ) {
             int dir_index = dir_ls[x] != "" ? dir_ls[x].toInt() : (-1);
              if ( dir_index > -1 ) {
                  NodeButton* button = this->directories_.value(dir_index);
                  button->show();
                 this->temp.push_back(button);
              }
          }
      }
      create_interface();
}

int VirtualConcierge::get_index_from_index(QVector<NodeButton* > list,
                                           int index) {
    int temp = -1;
    for ( int l = 0; l < list.count(); l++ )
        list.value(l)->getIndex() == index ? temp = l : temp = temp;

    return temp;
}

void VirtualConcierge::create_interface() {
    const int width = 256, height = 48;
    // reconstruct the directories_
    for ( int k = 0; k < this->catagory_.count(); k++ )
       this->catagory_.value(k)->setGeometry(0,
                                            k * (height+5),
                                            width,
                                            height);

    // reconstruct temp buttons
    for (  int z = 0; z <this->temp.count(); z++ )
       this->temp.value(z)->setGeometry(0,
                                        z *  (height+5),
                                        width,
                                        height);
}

void VirtualConcierge::load_interface(QString filename,
                                      QString filename_directories_) {
    // whenever the directories_ file does not exist, just add some buttons_

    // load the text file
    QFile textfile(filename);

    // open the text file
    textfile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream ascread(&textfile);

        if ( textfile.isOpen() ) {
            // read each line of the file
            QString line = ascread.readLine();

            while ( !line.isNull() ) {
                // break the line up in usable parts
                QStringList list = line.split(",");

                // check the type of line
                /* n-> node
                 * j-> join
                 */
                if ( list[0] == "n" ) {
                    QString name ="";
                    int index = 0, add = 0;
                    name = list[5];
                    QTextStream(&list[6]) >> add;
                    QTextStream(&list[1]) >> index;
                    if ( add == 1 ) {
                        NodeButton *button =
                                new NodeButton(ui->widget_directory);
                        button->setText(name);
                        button->setIndex(index);
                        const int width = 256, height = 48;
                        button->setGeometry(
                                    1,
                                    this->buttons_.count() *
                                    (height + 5),
                                    width,
                                    height);
                        PremisesExporter::fileExists(filename_directories_) ?
                                    button->hide() : button->show();
                        connect(button, SIGNAL(clicked_index(int, bool)),
                                this, SLOT(get_button_value(int, bool)));
                        QFont font = button->font();
                        font.setPointSize(12);
                        button->setFont(font);
                       this->buttons_.push_back(button);
                    }
                }
                // read next line
               line = ascread.readLine();
            }

            // close the textfile
            textfile.close();
        }

        if ( PremisesExporter::fileExists(filename_directories_) ) {
            // clear directory list
           this->directory_list_.clear();
           this->node_list_.clear();
           this->catagory_.clear();
           this->directories_.clear();
            // load the text file
            QFile textfile_directories_(filename_directories_);

            // open the text file
            textfile_directories_.open(QIODevice::ReadOnly | QIODevice::Text);
            QTextStream ascread(&textfile_directories_);

            if ( textfile_directories_.isOpen() ) {
                // read each line of the file
                QString line = ascread.readLine();
                int count_top = 0;
                while ( !line.isNull() ) {
                    // break the line up in usable parts
                    QStringList list = line.split(",");

                    if ( list[0] == "dd" ) {
                        QStringList dir = list[1].split(":");
                        QString name = dir.count() > 1 ? dir[1] : list[1];
                        int index = list[2].toInt();
                        const int width = 256, height = 48;
                        NodeButton *button =
                                new NodeButton(ui->widget_directory);
                        button->setText(name);
                        button->setIndex(index);
                        button->setDirectory(dir.count() > 1);
                        button->setGeometry(
                                    1,
                                    this->catagory_.count() *
                                    (height + 5),
                                    width,
                                    height);
                        QFont font = button->font();
                        font.setPointSize(12);
                        button->setFont(font);
                        button->show();
                       connect(button, SIGNAL(clicked_index(int, bool)),
                               this, SLOT(get_button_value(int, bool)));
                       this->catagory_.push_back(button);
                    } else if ( list[0] == "dl" ) {
                           this->directory_list_.append(list[3]);
                           this->node_list_.append(list[2]);
                    } else if ( list[0] == "d" ) {
                        QStringList dir = list[1].split(":");
                        QString name = dir.count() > 1 ?
                                    dir[1] : list[1];
                        NodeButton *button =
                                new NodeButton(ui->widget_directory);
                        button->setText(name);
                        button->setIndex(count_top);
                        button->setDirectory(dir.count() > 1);
                        button->setGeometry(
                                    1,
                                    this->directories_.count() *
                                    (button->height() + 1),
                                    button->width(),
                                    button->height());
                        connect(button, SIGNAL(clicked_index(int, bool)),
                                this, SLOT(get_button_value(int, bool)));
                        QFont font = button->font();
                        font.setPointSize(12);
                        button->setFont(font);
                        button->hide();
                       this->directories_.push_back(button);
                        count_top++;
                    }

                    // read next line
                   line = ascread.readLine();
                }

                // close the textfile
                textfile_directories_.close();
            }
        }
}

void VirtualConcierge::on_pushButton_send_mail_clicked() {
    QImage img = ui->openGLWidget->grabFramebuffer();
    img.save("FloorPlan.jpg");
    Smtp* smtp = new Smtp("virtualconcierge@yahoo.com",
                          "sel_foon@1",
                          "smtp.mail.yahoo.com",
                          465,
                          30000);
    QStringList ls;
    ls.append("FloorPlan.jpg");
    smtp->sendMail("virtualconcierge@yahoo.com",
                   ui->lineEdit_email->text(),
                   "This is a subject",
                   "This is a body",
                   ls);
}

VirtualConcierge::~VirtualConcierge() {
    delete ui;
    delete reset_timer;
}

void VirtualConcierge::on_button_wheelchair_clicked() {
    emit send_access(ui->button_wheelchair->isChecked(),
                     ui->button_feet->isChecked(),
                     ui->button_bicycle->isChecked(),
                     ui->button_other_vehicle->isChecked());
}

void VirtualConcierge::on_button_feet_clicked(){
    emit send_access(ui->button_wheelchair->isChecked(),
                     ui->button_feet->isChecked(),
                     ui->button_bicycle->isChecked(),
                     ui->button_other_vehicle->isChecked());
}

void VirtualConcierge::on_button_bicycle_clicked() {
    emit send_access(ui->button_wheelchair->isChecked(),
                     ui->button_feet->isChecked(),
                     ui->button_bicycle->isChecked(),
                     ui->button_other_vehicle->isChecked());
}

void VirtualConcierge::on_button_other_vehicle_clicked() {
    emit send_access(ui->button_wheelchair->isChecked(),
                     ui->button_feet->isChecked(),
                     ui->button_bicycle->isChecked(),
                     ui->button_other_vehicle->isChecked());
}

void VirtualConcierge::on_button_play_pause_clicked(bool checked) {
  // change text according to the checked value
  if ( checked ) {
      ui->button_play_pause->setText("Paused");
  } else {
      ui->button_play_pause->setText("Playing");
  }
  emit pause(checked);
}
