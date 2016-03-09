#include "utils/PreferencesManager.h"
#include <QFile>
#include <QDir>
#include <QSettings>

PreferencesManager& PreferencesManager::instance()
{
  static PreferencesManager instance;
  return instance;
}

PreferencesManager::PreferencesManager()
{

}

PreferencesManager::~PreferencesManager()
{

}

QString PreferencesManager::getImagesRootDirectory() const
{
  QSettings settings;
  return settings.value("imagesRootDirectory", "images").toString();
}

void PreferencesManager::setImagesRootDirectory(
    const QString& imagesRootDirectory)
{
  QSettings settings;
  settings.setValue("imagesRootDirectory", imagesRootDirectory);
}

QString PreferencesManager::getDatabaseFilePath() const
{
  QSettings settings;
  return settings.value("databaseFilePath", "annotation.sqlite").toString();
}

void PreferencesManager::setDatabaseFilePath(const QString &databaseFilePath)
{
  QSettings settings;
  settings.setValue("databaseFilePath", databaseFilePath);
}
