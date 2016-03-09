#include "gui/MainWindow.h"
#include "gui/PreferencesDialog.h"
#include "utils/PreferencesManager.h"
#include "utils/util_functions.h"
#include <QVector>
#include <QMenuBar>
#include <QImageReader>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextCodec>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDebug>

using namespace psa;

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
  setCodecs("UTF-8");
  setWindowTitle(tr("行人搜索标注工具"));

  createMenus();
  createPanels();

  loadDatabase(PreferencesManager::instance().getDatabaseFilePath());
}

MainWindow::~MainWindow()
{

}

void MainWindow::closeEvent(QCloseEvent* event)
{
  save();
  event->accept();
}

void MainWindow::openFolder()
{
  const PreferencesManager& pm = PreferencesManager::instance();
  QString folderPath = QFileDialog::getExistingDirectory(
      this, tr("打开图片文件夹"), pm.getImagesRootDirectory(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!isValidFolder(QDir(pm.getImagesRootDirectory()).absolutePath(),
                     QDir(folderPath).absolutePath())) {
    QMessageBox::critical(this, tr("无法打开文件夹"),
        tr("文件夹必须存放在根目录下"),
        QMessageBox::Ok);
    return;
  }
  loadFolder(folderPath);
}

void MainWindow::openDatabase()
{
  const PreferencesManager& pm = PreferencesManager::instance();
  QString filePath = QFileDialog::getOpenFileName(
      this, tr("打开标注文件"), pm.getDatabaseFilePath(),
      tr("标注数据库 (*.sqlite)"));
  if (filePath.isEmpty()) return;
  loadDatabase(filePath);
}

void MainWindow::editPreferences()
{
  PreferencesDialog preferencesDialog;
  preferencesDialog.exec();
}

void MainWindow::save()
{
  databaseHelper_.syncPersonBBoxes(annotationArea_->getPersonBBoxes(),
                                   annotationArea_->getRemovedMarks());
}

void MainWindow::exportToPersonTxt()
{
  QString filePath = QFileDialog::getSaveFileName(
      this, tr("导出为 按行人标注"), "person_annotation.txt");
  if (filePath.isEmpty()) return;
  databaseHelper_.exportToPersonTxt(filePath);
}

void MainWindow::exportToImageTxt()
{
  QString filePath = QFileDialog::getSaveFileName(
      this, tr("导出为 按图片标注"), "image_annotation.txt");
  if (filePath.isEmpty()) return;
  databaseHelper_.exportToImageTxt(filePath);
}

void MainWindow::modeAction()
{
  QAction* action = static_cast<QAction*>(sender());
  ImageArea::Mode mode = actionModeMap_.value(action);
  annotationArea_->setMode(mode);
}

void MainWindow::nextAction()
{
  annotationGalleryNavigator_->next();
}

void MainWindow::prevAction()
{
  annotationGalleryNavigator_->prev();
}

void MainWindow::toggleHardAction()
{
  annotationArea_->toggleHard();
}

void MainWindow::viewNavigateTo(int /* index */, const ImageFile& imageFile)
{  
  QDir root(PreferencesManager::instance().getImagesRootDirectory());
  QString absPath = root.filePath(imageFile.getPath());
  QImageReader imageReader(absPath);
  imageReader.setAutoTransform(true);
  viewArea_->setImage(imageReader.read(), imageFile.getImageId());
  viewArea_->setPersonBBoxes(
      databaseHelper_.getPersonBBoxesByImageId(imageFile.getImageId()));
}

void MainWindow::annotationNavigateTo(int index, const ImageFile& imageFile)
{
  save();
  // Show next image file
  QDir root(PreferencesManager::instance().getImagesRootDirectory());
  QString absPath = root.filePath(imageFile.getPath());
  QImageReader imageReader(absPath);
  imageReader.setAutoTransform(true);
  annotationArea_->setImage(imageReader.read(), imageFile.getImageId());
  annotationArea_->setPersonBBoxes(
      databaseHelper_.getPersonBBoxesByImageId(imageFile.getImageId()));
  viewGalleryNavigator_->navigate(index > 0 ? index - 1 : 0);
}

void MainWindow::viewPersonBBoxSelected()
{
  PersonBBox viewPersonBBox = viewArea_->getSelectedPersonBBox();
  PersonBBox annotationPersonBBox = annotationArea_->getSelectedPersonBBox();
  if (viewPersonBBox.isNull() || annotationPersonBBox.isNull()) return;
  if (viewPersonBBox.getPersonId() <= 0) return;
  annotationArea_->setPersonIdOfSelectedBBox(viewPersonBBox.getPersonId());
  viewArea_->clearSelectionAfterMouseReleased();
  annotationArea_->clearSelection();
}

void MainWindow::annotationPersonBBoxSelected()
{
  PersonBBox viewPersonBBox = viewArea_->getSelectedPersonBBox();
  PersonBBox annotationPersonBBox = annotationArea_->getSelectedPersonBBox();
  if (viewPersonBBox.isNull() || annotationPersonBBox.isNull()) return;
  if (viewPersonBBox.getPersonId() <= 0) return;
  annotationArea_->setPersonIdOfSelectedBBox(viewPersonBBox.getPersonId());
  viewArea_->clearSelection();
  annotationArea_->clearSelectionAfterMouseReleased();
}

void MainWindow::setCodecs(const char* codec)
{
  QTextCodec::setCodecForLocale(QTextCodec::codecForName(codec));
}

void MainWindow::createMenus()
{
  QMenu* fileMenu = menuBar()->addMenu(tr("&文件"));
  QAction* openFolderAction = fileMenu->addAction(tr("打开图片文件夹"));
  openFolderAction->setShortcut(QKeySequence::Open);
  connect(openFolderAction, &QAction::triggered, this, &MainWindow::openFolder);
  QAction* openDatabaseAction = fileMenu->addAction(tr("打开标注数据库"));
  connect(openDatabaseAction, &QAction::triggered, this, &MainWindow::openDatabase);
  QAction* saveAction = fileMenu->addAction(tr("保存"));
  saveAction->setShortcut(QKeySequence::Save);
  connect(saveAction, &QAction::triggered, this, &MainWindow::save);
  fileMenu->addSeparator();
  QAction* exportToPersonTxtAction = fileMenu->addAction(
      tr("导出为 按行人标注"));
  connect(exportToPersonTxtAction, &QAction::triggered,
          this, &MainWindow::exportToPersonTxt);
  QAction* exportToImageTxtAction = fileMenu->addAction(
      tr("导出为 按图片标注"));
  connect(exportToImageTxtAction, &QAction::triggered,
          this, &MainWindow::exportToImageTxt);

  QMenu* editMenu = menuBar()->addMenu(tr("&编辑"));
  QAction* editPreferencesAction = editMenu->addAction(tr("选项"));
  connect(editPreferencesAction, &QAction::triggered,
          this, &MainWindow::editPreferences);

  QMenu* annoMenu = menuBar()->addMenu(tr("&标注"));
  QAction* selectionAction = annoMenu->addAction(tr("选择行人模式"));
  selectionAction->setCheckable(true);
  selectionAction->setShortcut(QKeySequence("S"));
  connect(selectionAction, &QAction::triggered,
          this, &MainWindow::modeAction);
  QAction* headFeetAction = annoMenu->addAction(tr("头脚标注模式"));
  headFeetAction->setCheckable(true);
  headFeetAction->setShortcut(QKeySequence("A"));
  connect(headFeetAction, &QAction::triggered,
          this, &MainWindow::modeAction);
  QAction* dragDropAction = annoMenu->addAction(tr("拖拽标注模式"));
  dragDropAction->setCheckable(true);
  dragDropAction->setShortcut(QKeySequence("D"));
  connect(dragDropAction, &QAction::triggered,
          this, &MainWindow::modeAction);
  QActionGroup* modeActionGroup = new QActionGroup(this);
  modeActionGroup->addAction(selectionAction);
  modeActionGroup->addAction(headFeetAction);
  modeActionGroup->addAction(dragDropAction);
  actionModeMap_.insert(selectionAction, ImageArea::ModeSelection);
  actionModeMap_.insert(headFeetAction, ImageArea::ModeAnnotationByHeadFeat);
  actionModeMap_.insert(dragDropAction, ImageArea::ModeAnnotationByDragDrop);
  annoMenu->addSeparator();
  QAction* prevAction = annoMenu->addAction(tr("前一张"));
  prevAction->setShortcut(QKeySequence("Q"));
  connect(prevAction, &QAction::triggered, this, &MainWindow::prevAction);
  QAction* nextAction = annoMenu->addAction(tr("后一张"));
  nextAction->setShortcut(QKeySequence("W"));
  connect(nextAction, &QAction::triggered, this, &MainWindow::nextAction);
  annoMenu->addSeparator();
  QAction* toggleHardAction = annoMenu->addAction(tr("标记 / 取消标记 为困难的样本"));
  toggleHardAction->setShortcut(QKeySequence("Z"));
  connect(toggleHardAction, &QAction::triggered,
          this, &MainWindow::toggleHardAction);
}

void MainWindow::createPanels()
{
  viewGalleryNavigator_ = new GalleryNavigator;
  annotationGalleryNavigator_ = new GalleryNavigator;
  viewArea_ = new ImageArea;
  viewArea_->setPermissionFlags(ImageArea::AllowSelection);
  annotationArea_ = new ImageArea;
  annotationArea_->setPermissionFlags(ImageArea::AllowSelection |
                                      ImageArea::AllowAnnotation |
                                      ImageArea::AllowMoving |
                                      ImageArea::AllowResizing |
                                      ImageArea::AllowRemoving);

  connect(viewGalleryNavigator_, &GalleryNavigator::navigateTo,
          this, &MainWindow::viewNavigateTo);
  connect(annotationGalleryNavigator_, &GalleryNavigator::navigateTo,
          this, &MainWindow::annotationNavigateTo);
  connect(viewArea_, &ImageArea::personBBoxSelected,
          this, &MainWindow::viewPersonBBoxSelected);
  connect(annotationArea_, &ImageArea::personBBoxSelected,
          this, &MainWindow::annotationPersonBBoxSelected);

  QVBoxLayout* viewPanelLayout = new QVBoxLayout;
  viewPanelLayout->addWidget(viewGalleryNavigator_);
  viewPanelLayout->addWidget(viewArea_);

  QVBoxLayout* annotationPanelLayout = new QVBoxLayout;
  annotationPanelLayout->addWidget(annotationGalleryNavigator_);
  annotationPanelLayout->addWidget(annotationArea_);

  QWidget* mainFrame = new QWidget;
  QHBoxLayout* mainFrameLayout = new QHBoxLayout;
  mainFrameLayout->addLayout(viewPanelLayout);
  mainFrameLayout->addLayout(annotationPanelLayout);
  mainFrame->setLayout(mainFrameLayout);

  setCentralWidget(mainFrame);
  showMaximized();
}

void MainWindow::loadFolder(const QString& folderPath)
{
  // Get folder's relative path to the root directory
  const PreferencesManager& pm = PreferencesManager::instance();
  QDir root(pm.getImagesRootDirectory());
  QString relPath = root.relativeFilePath(folderPath);
  // Use its prefix as the author
  QString prefix = relPath.split("/", QString::SkipEmptyParts).front();

  QStringList paths;
  listImageFiles(folderPath, &paths);
  for (int i = 0; i < paths.size(); ++i) {
    paths[i] = root.relativeFilePath(paths[i]);
  }
  QVector<ImageFile> imageFiles = databaseHelper_.addAndQueryImageFiles(
      paths, prefix);

  viewGalleryNavigator_->setImageFiles(imageFiles);
  annotationGalleryNavigator_->setImageFiles(imageFiles);
  annotationGalleryNavigator_->navigate(0);
  actionModeMap_.key(ImageArea::ModeSelection)->trigger();
}

void MainWindow::loadDatabase(const QString& filePath)
{
  // Save current annotation
  save();
  // Reset widgets
  viewGalleryNavigator_->reset();
  annotationGalleryNavigator_->reset();
  viewArea_->reset();
  annotationArea_->reset();
  // Load new database
  databaseHelper_.init(filePath);
  // Set this database as the default one
  PreferencesManager::instance().setDatabaseFilePath(filePath);
  setWindowTitle(tr("行人搜索标注工具 - ") + QFileInfo(filePath).fileName());
}

bool MainWindow::isValidFolder(const QString& root, const QString& folder)
{
  if (folder.isEmpty()) return false;
  if (folder == root) return false;
  if (QDir(root).relativeFilePath(folder).startsWith("..")) return false;
  return true;
}
