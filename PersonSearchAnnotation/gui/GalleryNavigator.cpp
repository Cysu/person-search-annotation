#include "gui/GalleryNavigator.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>

GalleryNavigator::GalleryNavigator(QWidget* parent)
  : QWidget(parent),
    currentIndex_(-1)
{
  createPanels();
}

void GalleryNavigator::reset()
{
  currentIndex_ = -1;
  imageFiles_.clear();
  jumpToEdit_->setText("");
  infoLabel_->setText("");
}

QVector<ImageFile> GalleryNavigator::getImageFiles() const
{
  return imageFiles_;
}

void GalleryNavigator::setImageFiles(const QVector<ImageFile>& imageFiles)
{
  imageFiles_ = imageFiles;
  currentIndex_ = 0;
  updateInfo();
}

int GalleryNavigator::getCurrentIndex() const
{
  return currentIndex_;
}

ImageFile GalleryNavigator::getCurrentImageFile() const
{
  return imageFiles_[currentIndex_];
}

void GalleryNavigator::navigate(int index)
{
  if (index < 0 || index >= static_cast<int>(imageFiles_.size())) {
    QMessageBox::critical(this, tr("无法跳转"),
                          tr("错误的图片序号"),
                          QMessageBox::Ok);
    return;
  }
  currentIndex_ = index;
  updateInfo();
  emit navigateTo(currentIndex_, imageFiles_[currentIndex_]);
}

void GalleryNavigator::jump()
{
  navigate(jumpToEdit_->text().toInt() - 1);
}

void GalleryNavigator::next()
{
  navigate(currentIndex_ + 1);
}

void GalleryNavigator::prev()
{
  navigate(currentIndex_ - 1);
}

void GalleryNavigator::createPanels()
{
  QIcon leftArrowIcon(":/icons/left_arrow.png");
  QPushButton* prevButton = new QPushButton(leftArrowIcon, tr(""));
  prevButton->setMaximumWidth(40);
  prevButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  QIcon rightArrowIcon(":/icons/right_arrow.png");
  QPushButton* nextButton = new QPushButton(rightArrowIcon, tr(""));
  nextButton->setMaximumWidth(40);
  nextButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  jumpToEdit_ = new QLineEdit;
  jumpToEdit_->setMaximumWidth(60);
  jumpToEdit_->setAlignment(Qt::AlignRight);
  infoLabel_ = new QLabel;

  connect(prevButton, &QPushButton::clicked, this, &GalleryNavigator::prev);
  connect(nextButton, &QPushButton::clicked, this, &GalleryNavigator::next);
  connect(jumpToEdit_, &QLineEdit::returnPressed, this, &GalleryNavigator::jump);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addStretch(3);
  layout->addWidget(prevButton);
  layout->addWidget(jumpToEdit_);
  layout->addWidget(infoLabel_);
  layout->addWidget(nextButton);
  layout->addStretch(3);
  setLayout(layout);
}

void GalleryNavigator::updateInfo()
{
  jumpToEdit_->setText(QString::number(currentIndex_ + 1));
  infoLabel_->setText(QString(" / %1").arg(imageFiles_.size()));
}
