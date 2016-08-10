#include "./login.h"
#include "./ui_login.h"
#include <QProgressBar>
#include <QThread>

login::login(QWidget *parent) :
    QWidget(parent),
    logged_in_(false),
    session_(""),
    ui(new Ui::login) {
    ui->setupUi(this);
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
    ui->lineEdit_password->setInputMethodHints(Qt::ImhHiddenText |
                                               Qt::ImhNoPredictiveText |
                                               Qt::ImhNoAutoUppercase);
    spinner = new WaitingSpinnerWidget(this, Qt::ApplicationModal, true);
    ui->progressBar->setVisible(false);
    ui->label_download->setVisible(false);
    main_program = new MainWindow();
    main_program->showMinimized();
}

login::~login() {
    delete ui;
}

void login::on_pushButton_login_clicked() {
    client_logging = new Client();
    connect(client_logging, SIGNAL(logged_in(QByteArray, bool)),
            this, SLOT(logged_in(QByteArray, bool)));
    connect(client_logging, SIGNAL(download_progress(int,int)),
            this, SLOT(download_progress(int,int)));
    client_logging->Login(ui->lineEdit_username->text(),
                          ui->lineEdit_password->text());
    //client_logging->send_file("session", "VirtualConcierge/Nodes.pvc");
    //client_logging->send_file("session", "VirtualConcierge/TEX0");
    spinner->start(); // starts spinning
}

bool login::clearDir( const QString path )
{
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

void login::logged_in(QByteArray session, bool value) {
  this->logged_in_ = value;
  this->session_ = session;
  spinner->stop();
  if( value ) {
    ui->label_download->setVisible(true);
    ui->label_download->setText("Log-In Successful");
    while(!clearDir("VirtualConcierge"));
  } else {
    ui->label_download->setVisible(true);
    ui->label_download->setText("Log-In Fail Username of Password incorrect");
  }
}

bool login::get_logged() {
  return logged_in_;
}

void login::download_progress(int count, int max) {
  ui->progressBar->setVisible(true);
  ui->progressBar->setMaximum(max);
  ui->progressBar->setValue(count);
  ui->progressBar->update();
  // label
  int progress_p = (100 *(float)count / (float)max);
  QString progress = QString("Downloading...%1").arg(QString::number(progress_p));
  progress.append("%");
  ui->label_download->setVisible(true);
  ui->label_download->setText(progress);
  if ( count == max ) {
      ui->label_download->setText("Download Complete.");
      spinner->start();

      QDirIterator dirIt_check("VirtualConcierge", QDirIterator::Subdirectories);
      int dir_count = 0;
      while (dirIt_check.hasNext()) {
        dirIt_check.next();
        if (QFileInfo(dirIt_check.filePath()).isFile()) {
            dir_count++;
        }
      }

     // while(client_logging->busy());
      main_program->close();
      MainWindow *window_main = new MainWindow();
      connect(this, SIGNAL(log_to_main(QByteArray, bool)),
              window_main, SLOT(receive_session(QByteArray,bool)));\
      emit log_to_main(session_, true);
      window_main->showMaximized();

      this->close();
  }
}

void login::on_pushButton_terminate_clicked() {
    // completely exit the program
    QApplication::exit(0);
}

void login::on_pushButton_close_clicked() {

  spinner->start();
 // spinner->show();
  //MainWindow *w = new MainWindow();
  main_program->showMaximized();

  // close the window
  this->close();
}
