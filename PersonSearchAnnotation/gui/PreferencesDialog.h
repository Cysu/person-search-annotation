#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QLineEdit>

class PreferencesDialog : public QDialog
{
  Q_OBJECT

public:
  explicit PreferencesDialog(QWidget* parent = 0);
  ~PreferencesDialog();

public slots:
  void save();

private:
  QLineEdit* imagesRootDirectory_;

private:
  void chooseImagesRoot();
  void createPanels();
  void loadPreferences();
};

#endif // PREFERENCESDIALOG_H
