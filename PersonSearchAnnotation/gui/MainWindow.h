#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "common/ImageFile.hpp"
#include "common/PersonBBox.hpp"
#include "gui/GalleryNavigator.h"
#include "gui/ImageArea.h"
#include "db/DatabaseHelper.h"
#include <QMap>
#include <QMainWindow>

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

protected:
  void closeEvent(QCloseEvent* event);

private slots:
  void openFolder();
  void openDatabase();
  void editPreferences();
  void save();
  void exportToPersonTxt();
  void exportToImageTxt();

  void modeAction();
  void nextAction();
  void prevAction();
  void toggleHardAction();

  void viewNavigateTo(int index, const ImageFile& imageFile);
  void annotationNavigateTo(int index, const ImageFile& imageFile);
  void viewPersonBBoxSelected();
  void annotationPersonBBoxSelected();

private:
  void setCodecs(const char* codec = "UTF-8");
  void createMenus();
  void createPanels();

  void loadFolder(const QString& folderPath);
  void loadDatabase(const QString& filePath);

  bool isValidFolder(const QString& root, const QString& folder);

private:
  QMap<QAction*, ImageArea::Mode> actionModeMap_;

  GalleryNavigator* viewGalleryNavigator_;
  GalleryNavigator* annotationGalleryNavigator_;

  ImageArea* viewArea_;
  ImageArea* annotationArea_;

  DatabaseHelper databaseHelper_;
};

#endif // MAINWINDOW_H
