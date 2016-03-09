#ifndef PREFERENCESMANAGER_H
#define PREFERENCESMANAGER_H

#include <QString>

class PreferencesManager
{
public:
  static PreferencesManager& instance();

  QString getImagesRootDirectory() const;
  void setImagesRootDirectory(const QString& imagesRootDirectory);

  QString getDatabaseFilePath() const;
  void setDatabaseFilePath(const QString& databaseFilePath);

private:
  PreferencesManager();
  PreferencesManager(const PreferencesManager&);
  const PreferencesManager& operator = (const PreferencesManager&);
  ~PreferencesManager();
};

#endif // PREFERENCESMANAGER_H
