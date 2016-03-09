#ifndef DATABASEHELPER_H
#define DATABASEHELPER_H

#include "common/ImageFile.hpp"
#include "common/PersonBBox.hpp"
#include <QVector>
#include <QStringList>
#include <QSqlDatabase>

class DatabaseHelper
{
public:
  DatabaseHelper();
  ~DatabaseHelper();

  void init(const QString& filePath);
  void exportToPersonTxt(const QString& filePath);
  void exportToImageTxt(const QString& filePath);

  ImageFile getImageFile(const QString& path);

  PersonBBox getPersonBBox(int bboxId);
  QVector<PersonBBox> getPersonBBoxesByImageId(int imageId);
  QVector<PersonBBox> getPersonBBoxesByPersonId(int personId);

  bool hasPerson(int personId);

  int addImageFile(const QString& path, const QString& author);
  int addPerson(int personId);
  int addPersonBBox(const PersonBBox& personBBox);

  void removePersonBBox(int bboxId);

  QVector<ImageFile> addAndQueryImageFiles(
      const QStringList& paths, const QString& author);

  void syncPersonBBoxes(const QVector<PersonBBox>& personBBoxes,
                        const QVector<bool>& removedMarks);

private:
  void createTables();

private:
  QSqlDatabase db_;
};

#endif // DATABASEHELPER_H
