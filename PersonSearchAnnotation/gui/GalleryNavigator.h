#ifndef GALLERYNAVIGATOR_H
#define GALLERYNAVIGATOR_H

#include "common/ImageFile.hpp"
#include <QVector>
#include <QWidget>
#include <QStringList>

class QLineEdit;
class QLabel;

class GalleryNavigator : public QWidget
{
  Q_OBJECT

public:
  explicit GalleryNavigator(QWidget* parent = 0);

  void reset();

  QVector<ImageFile> getImageFiles() const;
  void setImageFiles(const QVector<ImageFile>& imageFiles);

  int getCurrentIndex() const;
  ImageFile getCurrentImageFile() const;

public slots:
  void jump();
  void next();
  void prev();
  void navigate(int index);

signals:
  void navigateTo(int index, ImageFile imageFile);

private:
  void createPanels();
  void updateInfo();

private:
  QLineEdit* jumpToEdit_;
  QLabel* infoLabel_;

  QVector<ImageFile> imageFiles_;
  int currentIndex_;
};

#endif // GALLERYNAVIGATOR_H
