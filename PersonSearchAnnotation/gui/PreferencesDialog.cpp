#include "gui/PreferencesDialog.h"
#include "utils/PreferencesManager.h"
#include <QIcon>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

PreferencesDialog::PreferencesDialog(QWidget* parent)
  : QDialog(parent)
{
  createPanels();
  loadPreferences();
}

PreferencesDialog::~PreferencesDialog()
{

}

void PreferencesDialog::save()
{
  PreferencesManager& pm = PreferencesManager::instance();
  pm.setImagesRootDirectory(imagesRootDirectory_->text());
  close();
}

void PreferencesDialog::chooseImagesRoot()
{
  QString rootDir = QFileDialog::getExistingDirectory(
      this, tr("选择图片文件夹根目录"), imagesRootDirectory_->text());

  if (!rootDir.isEmpty()) {
      imagesRootDirectory_->setText(rootDir);
  }
}

void PreferencesDialog::createPanels()
{
  imagesRootDirectory_ = new QLineEdit;

  QIcon folderOpenIcon(":/icons/folder_open.png");
  QPushButton* imagesRootDirectoryButton = new QPushButton(folderOpenIcon, tr(""));
  connect(imagesRootDirectoryButton, &QPushButton::clicked,
          this, &PreferencesDialog::chooseImagesRoot);

  QPushButton* saveButton = new QPushButton(tr("保存"));
  QPushButton* cancelButton = new QPushButton(tr("取消"));
  connect(saveButton, &QPushButton::clicked, this, &PreferencesDialog::save);
  connect(cancelButton, &QPushButton::clicked, this, &PreferencesDialog::close);

  QGridLayout* layout = new QGridLayout;
  layout->addWidget(new QLabel(tr("图片文件夹根目录")), 0, 0);
  layout->addWidget(imagesRootDirectory_, 0, 1, 1, 2);
  layout->addWidget(imagesRootDirectoryButton, 0, 3);
  layout->addWidget(saveButton, 1, 0, 1, 2);
  layout->addWidget(cancelButton, 1, 2, 1, 2);
  setLayout(layout);
}

void PreferencesDialog::loadPreferences()
{
  PreferencesManager& pm = PreferencesManager::instance();
  imagesRootDirectory_->setText(pm.getImagesRootDirectory());
}

