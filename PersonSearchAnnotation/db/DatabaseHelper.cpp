#include "db/DatabaseHelper.h"
#include <cstdio>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

DatabaseHelper::DatabaseHelper()
  : db_(QSqlDatabase::addDatabase("QSQLITE"))
{

}

DatabaseHelper::~DatabaseHelper()
{
  db_.close();
}

void DatabaseHelper::init(const QString& filePath)
{
  db_.close();
  db_.setDatabaseName(filePath);
  db_.open();
  createTables();
}

void DatabaseHelper::exportToPersonTxt(const QString& filePath)
{
  // Get all people
  QSqlQuery query;
  query.prepare("SELECT person_id FROM psa_person");
  query.exec();
  QVector<int> personIds;
  while (query.next()) {
    personIds.push_back(query.value(0).toInt());
  }
  // Save to file
  FILE* fid = fopen(filePath.toStdString().c_str(), "w");
  for (int i = 0; i < personIds.size(); ++i) {
    query.prepare("SELECT psa_image.path, psa_bbox.x, psa_bbox.y,"
                  "       psa_bbox.width, psa_bbox.height, psa_bbox.hard "
                  "FROM psa_image, psa_bbox "
                  "WHERE psa_image.image_id = psa_bbox.image_id AND "
                  "      psa_bbox.person_id = :person_id");
    query.bindValue(":person_id", personIds[i]);
    query.exec();
    QVector<QString> contents;
    while (query.next()) {
      contents.push_back(query.value(0).toString() + "\t" +
                         QString::number(query.value(1).toInt()) + "\t" +
                         QString::number(query.value(2).toInt()) + "\t" +
                         QString::number(query.value(3).toInt()) + "\t" +
                         QString::number(query.value(4).toInt()) + "\t" +
                         QString::number(query.value(5).toInt()));
    }
    fprintf(fid, "# %d\n", personIds[i]);
    fprintf(fid, "%d\n", contents.size());
    for (int i = 0; i < contents.size(); ++i) {
      fprintf(fid, "%s\n", contents[i].toStdString().c_str());
    }
  }
  fclose(fid);
}

void DatabaseHelper::exportToImageTxt(const QString& filePath)
{
  // Get all images
  QSqlQuery query;
  query.prepare("SELECT image_id, path FROM psa_image");
  query.exec();
  QVector<int> imageIds;
  QVector<QString> imagePaths;
  while (query.next()) {
    imageIds.push_back(query.value(0).toInt());
    imagePaths.push_back(query.value(1).toString());
  }
  // Save to file
  FILE* fid = fopen(filePath.toStdString().c_str(), "w");
  for (int i = 0; i < imageIds.size(); ++i) {
    query.prepare("SELECT person_id, x, y, width, height, hard FROM psa_bbox "
                  "WHERE image_id = :image_id");
    query.bindValue(":image_id", imageIds[i]);
    query.exec();
    QVector<QString> contents;
    while (query.next()) {
      contents.push_back(QString::number(query.value(0).toInt()) + "\t" +
                         QString::number(query.value(1).toInt()) + "\t" +
                         QString::number(query.value(2).toInt()) + "\t" +
                         QString::number(query.value(3).toInt()) + "\t" +
                         QString::number(query.value(4).toInt()) + "\t" +
                         QString::number(query.value(5).toInt()));
    }
    fprintf(fid, "# %d\n", imageIds[i]);
    fprintf(fid, "%s\n", imagePaths[i].toStdString().c_str());
    fprintf(fid, "%d\n", contents.size());
    for (int i = 0; i < contents.size(); ++i) {
      fprintf(fid, "%s\n", contents[i].toStdString().c_str());
    }
  }
  fclose(fid);
}

ImageFile DatabaseHelper::getImageFile(const QString& path)
{
  QSqlQuery query;
  query.prepare("SELECT image_id, author FROM psa_image WHERE path = :path");
  query.bindValue(":path", path.toStdString().c_str());
  query.exec();

  ImageFile imageFile;
  if (!query.last()) return imageFile;
  imageFile.setImageId(query.value(0).toInt());
  imageFile.setPath(path);
  imageFile.setAuthor(query.value(1).toString());
  return imageFile;
}

PersonBBox DatabaseHelper::getPersonBBox(int bboxId)
{
  QSqlQuery query;
  query.prepare("SELECT image_id, person_id, x, y, width, height, hard "
                "FROM psa_bbox WHERE bbox_id = :bbox_id");
  query.bindValue(":bbox_id", bboxId);
  query.exec();

  PersonBBox personBBox;
  if (!query.last()) return personBBox;

  personBBox.setBBoxId(bboxId);
  personBBox.setImageId(query.value(0).toInt());
  personBBox.setPersonId(query.value(1).toInt());
  personBBox.setBBox(query.value(2).toInt(), query.value(3).toInt(),
                     query.value(4).toInt(), query.value(5).toInt());
  personBBox.setHard(query.value(6).toInt());

  return personBBox;
}

QVector<PersonBBox> DatabaseHelper::getPersonBBoxesByImageId(int imageId)
{
  QSqlQuery query;
  query.prepare("SELECT bbox_id, person_id, x, y, width, height, hard "
                "FROM psa_bbox WHERE image_id = :image_id");
  query.bindValue(":image_id", imageId);
  query.exec();

  QVector<PersonBBox> personBBoxes;
  while (query.next()) {
    PersonBBox personBBox;
    personBBox.setBBoxId(query.value(0).toInt());
    personBBox.setImageId(imageId);
    personBBox.setPersonId(query.value(1).toInt());
    personBBox.setBBox(query.value(2).toInt(), query.value(3).toInt(),
                       query.value(4).toInt(), query.value(5).toInt());
    personBBox.setHard(query.value(6).toInt());
    personBBoxes.push_back(personBBox);
  }
  return personBBoxes;
}

QVector<PersonBBox> DatabaseHelper::getPersonBBoxesByPersonId(int personId)
{
  QSqlQuery query;
  query.prepare("SELECT bbox_id, image_id, x, y, width, height, hard "
                "FROM psa_bbox WHERE person_id = :person_id");
  query.bindValue(":person_id", personId);
  query.exec();

  QVector<PersonBBox> personBBoxes;
  while (query.next()) {
    PersonBBox personBBox;
    personBBox.setBBoxId(query.value(0).toInt());
    personBBox.setImageId(query.value(1).toInt());
    personBBox.setPersonId(personId);
    personBBox.setBBox(query.value(2).toInt(), query.value(3).toInt(),
                       query.value(4).toInt(), query.value(5).toInt());
    personBBox.setHard(query.value(6).toInt());
    personBBoxes.push_back(personBBox);
  }
  return personBBoxes;
}

bool DatabaseHelper::hasPerson(int personId)
{
  QSqlQuery query;
  query.prepare("SELECT * FROM psa_person WHERE person_id = :person_id");
  query.bindValue(":person_id", personId);
  query.exec();
  return query.last();
}

int DatabaseHelper::addImageFile(const QString& path, const QString& author)
{
  QSqlQuery query;
  query.prepare("INSERT INTO psa_image(path, author) "
                "VALUES(:path, :author)");
  query.bindValue(":path", path.toStdString().c_str());
  query.bindValue(":author", author.toStdString().c_str());
  query.exec();
  return query.lastInsertId().toInt();
}

int DatabaseHelper::addPerson(int personId)
{
  QSqlQuery query;
  if (personId <= 0) {
    query.prepare("INSERT INTO psa_person DEFAULT VALUES");
  } else if (!hasPerson(personId)) {
    query.prepare("INSERT INTO psa_person(person_id) VALUES(:person_id)");
    query.bindValue(":person_id", personId);
  } else {
    return personId;
  }
  query.exec();
  return query.lastInsertId().toInt();
}

int DatabaseHelper::addPersonBBox(const PersonBBox& personBBox)
{
  QSqlQuery query;
  // Add person if not exists
  int personId = addPerson(personBBox.getPersonId());

  // Add person bbox
  if (personBBox.getBBoxId() <= 0) {
    query.prepare("INSERT INTO psa_bbox(image_id, person_id, x, y, width, height, hard) "
                  "VALUES(:image_id, :person_id, :x, :y, :width, :height, :hard)");
  } else {
    query.prepare("INSERT INTO psa_bbox(bbox_id, image_id, person_id, x, y, width, height, hard) "
                  "VALUES(:bbox_id, :image_id, :person_id, :x, :y, :width, :height, :hard)");
    query.bindValue(":bbox_id", personBBox.getBBoxId());
  }
  query.bindValue(":image_id", personBBox.getImageId());
  query.bindValue(":person_id", personId);
  query.bindValue(":x", personBBox.x());
  query.bindValue(":y", personBBox.y());
  query.bindValue(":width", personBBox.width());
  query.bindValue(":height", personBBox.height());
  query.bindValue(":hard", static_cast<int>(personBBox.isHard()));
  query.exec();
  return query.lastInsertId().toInt();
}

void DatabaseHelper::removePersonBBox(int bboxId)
{
  int personId = getPersonBBox(bboxId).getPersonId();
  QSqlQuery query;
  query.prepare("DELETE FROM psa_bbox WHERE bbox_id = :bbox_id");
  query.bindValue(":bbox_id", bboxId);
  query.exec();
  if (getPersonBBoxesByPersonId(personId).empty()) {
    query.prepare("DELETE FROM psa_person WHERE person_id = :person_id");
    query.bindValue(":person_id", personId);
    query.exec();
  }
}

QVector<ImageFile> DatabaseHelper::addAndQueryImageFiles(
    const QStringList &paths, const QString& author)
{
  QVector<ImageFile> ret;
  foreach (QString path, paths) {
    ImageFile imageFile = getImageFile(path);
    if (imageFile.isNull()) {
      imageFile.setPath(path);
      imageFile.setAuthor(author);
      imageFile.setImageId(addImageFile(path, author));
    }
    ret.push_back(imageFile);
  }
  return ret;
}

void DatabaseHelper::syncPersonBBoxes(const QVector<PersonBBox>& personBBoxes,
                                      const QVector<bool>& removedMarks)
{
  for (int i = 0; i < personBBoxes.size(); ++i) {
    const PersonBBox& personBBox = personBBoxes[i];
    if (removedMarks[i]) {
      removePersonBBox(personBBox.getBBoxId());
    } else {
      if (personBBox.getBBoxId() <= 0) {
        addPersonBBox(personBBox);
      } else {
        removePersonBBox(personBBox.getBBoxId());
        addPersonBBox(personBBox);
      }
    }
  }
}

void DatabaseHelper::createTables()
{
  QSqlQuery query;
  query.exec("PRAGMA foreign_keys = ON");
  query.exec("CREATE TABLE IF NOT EXISTS psa_image("
             "    image_id INTEGER PRIMARY KEY,"
             "    path TEXT NOT NULL,"
             "    author VARCHAR(128) NOT NULL)");
  query.exec("CREATE TABLE IF NOT EXISTS psa_person("
             "    person_id INTEGER PRIMARY KEY)");
  query.exec("CREATE TABLE IF NOT EXISTS psa_bbox("
             "    bbox_id INTEGER PRIMARY KEY,"
             "    image_id INTEGER NOT NULL,"
             "    person_id INTEGER NOT NULL,"
             "    x INTEGER NOT NULL,"
             "    y INTEGER NOT NULL,"
             "    width INTEGER NOT NULL,"
             "    height INTEGER NOT NULL,"
             "    hard INTEGER NOT NULL)");
}
